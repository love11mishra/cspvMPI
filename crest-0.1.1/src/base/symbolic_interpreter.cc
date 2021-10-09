// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
//
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

#include <algorithm>
#include <iostream>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <utility>
#include <vector>
#include <cstring>
#include <string> 	//@lavleshm 
#include <stack>	//@lavleshm
#include <sys/types.h>  
#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#include "base/symbolic_interpreter.h"
#include "base/yices_solver.h"

#include "mpi.h" //pkalita

using std::make_pair;
using std::swap;
using std::vector;
#ifdef DEBUG
#define IFDEBUG(x) x
#else
#define IFDEBUG(x)
#endif


namespace crest {

    typedef map<addr_t,SymbolicExpr*>::const_iterator ConstMemIt;


    SymbolicInterpreter::SymbolicInterpreter()
        : pred_(NULL), ex_(true), num_inputs_(0) {
            stack_.reserve(16);
            state_id = 0;
        }

    SymbolicInterpreter::SymbolicInterpreter(const vector<value_t>& input)
        : pred_(NULL), ex_(true) {
            stack_.reserve(16);
            ex_.mutable_inputs()->assign(input.begin(), input.end());
            state_id = 0;
        }

    void SymbolicInterpreter::DumpMemory() {
        FILE *tr;
        tr = fopen("trace.txt","a");
        for (ConstMemIt i = mem_.begin(); i != mem_.end(); ++i) {
            string s;
            i->second->AppendToString(&s);
            fprintf(tr, "%lu: %s [%d]\n", i->first, s.c_str(), *(int*)(i->first));
        }
        for (size_t i = 0; i < stack_.size(); i++) {
            string s;
            if (stack_[i].expr) {
                stack_[i].expr->AppendToString(&s);
            } else if ((i == stack_.size() - 1) && pred_) {
                pred_->AppendToString(&s);
            }
            fprintf(tr, "s%d: %lld [ %s ]\n", i, stack_[i].concrete, s.c_str());
        }

        fclose(tr);
    }


    void SymbolicInterpreter::ClearStack(id_t id) {
        FILE *tr;
        tr = fopen("trace.txt","a");
        IFDEBUG(fprintf(tr, "clear\n"));
        for (vector<StackElem>::const_iterator it = stack_.begin(); it != stack_.end(); ++it) {
            delete it->expr;
        }
        stack_.clear();
        ClearPredicateRegister();
        return_value_ = false;
        IFDEBUG(DumpMemory());
        fclose(tr);
    }


    void SymbolicInterpreter::Load(id_t id, addr_t addr, value_t value) {
        //FILE *tr;
        //tr = fopen("trace.txt","a");
        //IFDEBUG(fprintf(tr, "load %lu %lld\n", addr, value));
        ConstMemIt it;
        addr_t checkArrayElem = isArrayElemPresent(addr);
        if ((checkArrayElem != NULL)) {
//            PushSymbolic(new SymbolicExpr(checkArrayElem), value);
             it = mem_.find(checkArrayElem); // pkalita
        }else {
             it = mem_.find(addr); //original
        }


        if (it == mem_.end()) {
            PushConcrete(value);
        } else {
            PushSymbolic(new SymbolicExpr(*it->second), value);
        }

        //pkalita
        //it checks whether array element is part of any array defined using crest_intArray
        // and if it returns not null then pushing the starting address of the buffer to the stack


        ClearPredicateRegister();
        //IFDEBUG(DumpMemory());
        //fclose(tr);
    }


    void SymbolicInterpreter::Store(id_t id, addr_t addr) {
        FILE *tr;
        tr = fopen("trace.txt","a");
        IFDEBUG(fprintf(tr, "store %lu\n", addr));
        assert(stack_.size() > 0);

        const StackElem& se = stack_.back();
        if (se.expr) {
            if (!se.expr->IsConcrete()) {
                mem_[addr] = se.expr;
            } else {
                mem_.erase(addr);
                delete se.expr;
            }
        } else {
            mem_.erase(addr);
        }

        stack_.pop_back();
        ClearPredicateRegister();
        IFDEBUG(DumpMemory());
        fclose(tr);
    }


    void SymbolicInterpreter::ApplyUnaryOp(id_t id, unary_op_t op, value_t value) {
        FILE *tr;
        tr = fopen("trace.txt","a");
        IFDEBUG(fprintf(tr, "apply1 %d %lld\n", op, value));
        assert(stack_.size() >= 1);
        StackElem& se = stack_.back();

        if (se.expr) {
            switch (op) {
                case ops::NEGATE:
                    se.expr->Negate();
                    ClearPredicateRegister();
                    break;
                case ops::LOGICAL_NOT:
                    if (pred_) {
                        pred_->Negate();
                        break;
                    }
                    // Otherwise, fall through to the concrete case.
                default:
                    // Concrete operator.
                    delete se.expr;
                    se.expr = NULL;
                    ClearPredicateRegister();
            }
        }

        se.concrete = value;
        IFDEBUG(DumpMemory());
        fclose(tr);
    }


    void SymbolicInterpreter::ApplyBinaryOp(id_t id, binary_op_t op, value_t value) {
        FILE *tr;
        tr = fopen("trace.txt","a");
        IFDEBUG(fprintf(tr, "apply2 %d %lld\n", op, value));
        assert(stack_.size() >= 2);
        StackElem& a = *(stack_.rbegin()+1);
        StackElem& b = stack_.back();

        if (a.expr || b.expr) {
            switch (op) {
                case ops::ADD:
                    if (a.expr == NULL) {
                        swap(a, b);
                        *a.expr += b.concrete;
                    } else if (b.expr == NULL) {
                        *a.expr += b.concrete;
                    } else {
                        *a.expr += *b.expr;
                        delete b.expr;
                    }
                    break;

                case ops::SUBTRACT:
                    if (a.expr == NULL) {
                        b.expr->Negate();
                        swap(a, b);
                        *a.expr += b.concrete;
                    } else if (b.expr == NULL) {
                        *a.expr -= b.concrete;
                    } else {
                        *a.expr -= *b.expr;
                        delete b.expr;
                    }
                    break;

                case ops::MULTIPLY:
                    if (a.expr == NULL) {
                        swap(a, b);
                        *a.expr *= b.concrete;
                    } else if (b.expr == NULL) {
                        *a.expr *= b.concrete;
                    } else {
                        swap(a, b);
                        *a.expr *= b.concrete;
                        delete b.expr;
                    }
                    break;

                default:
                    // Concrete operator.
                    delete a.expr;
                    delete b.expr;
                    a.expr = NULL;
            }
        }

        a.concrete = value;
        stack_.pop_back();
        ClearPredicateRegister();
        IFDEBUG(DumpMemory());
        fclose(tr);
    }

    void SymbolicInterpreter::FreeMap(map<addr_t,string*> z)
    {
        for (map<addr_t,string*>::const_iterator it=z.begin(); it!=z.end(); it++)
        {
            if (it->second != NULL)
                delete it->second;
        }
    }

    void SymbolicInterpreter::ApplyCompareOp(id_t id, compare_op_t op, value_t value) {
        FILE *tr;
        tr = fopen("trace.txt","a");
        IFDEBUG(fprintf(tr, "compare2 %d %lld\n", op, value));
        assert(stack_.size() >= 2);
        StackElem& a = *(stack_.rbegin()+1);
        StackElem& b = stack_.back();
//        std::cout << "outside of a.expr || b.expr" << std::endl;
//        std::cout << "outside of a.concrete: " << a.concrete << " b.concrete: " << b.concrete << std::endl;
        if (a.expr || b.expr) {
//            std::cout << "inside of a.expr || b.expr" << std::endl;
            // Symbolically compute "a -= b".
            if (a.expr == NULL) {
                b.expr->Negate();
                swap(a, b);
                *a.expr += b.concrete;
            } else if (b.expr == NULL) {
                *a.expr -= b.concrete;
            } else {
                *a.expr -= *b.expr;
                delete b.expr;
            }
            // Construct a symbolic predicate (if "a - b" is symbolic), and
            // store it in the predicate register.
            if (!a.expr->IsConcrete()) {
//                std::cout << "inside of !a.expr->IsConcrete()" << std::endl;
                pred_ = new SymbolicPred(op, a.expr);
            } else {
//                std::cout << "false branch of !a.expr->IsConcrete()" << std::endl;
                ClearPredicateRegister();
                delete a.expr;
            }
            // We leave a concrete value on the stack.
            a.expr = NULL;
        }

        a.concrete = value;
        stack_.pop_back();
        IFDEBUG(DumpMemory());
        fclose(tr);
    }

    void SymbolicInterpreter::ClearAllMaps()
    {
        FreeMap(names_);
        FreeMap(names_trigger_);
        names_.clear();
        names_typs_.clear();
        names_trigger_.clear();
    }

    void SymbolicInterpreter::Call(id_t id, function_id_t fid) {
        ex_.mutable_path()->Push(kCallId);
        ClearAllMaps();
        //names_.clear(); // so that the local variable names in the caller don't persist
        // names_typs_.clear(); // so that the local variable names in the caller don't persist
        // names_trigger_.clear(); // so that the local variable names in the caller don't persist
    }


    void SymbolicInterpreter::Return(id_t id) {
        ex_.mutable_path()->Push(kReturnId);
        ClearAllMaps();

        // There is either exactly one value on the stack -- the current function's
        // return value -- or the stack is empty.
        assert(stack_.size() <= 1);

        return_value_ = (stack_.size() == 1);
    }


    void SymbolicInterpreter::HandleReturn(id_t id, value_t value) {
        if (return_value_) {
            // We just returned from an instrumented function, so the stack
            // contains a single element -- the (possibly symbolic) return value.
            assert(stack_.size() == 1);
            return_value_ = false;
        } else {
            // We just returned from an uninstrumented function, so the stack
            // still contains the arguments to that function.  Thus, we clear
            // the stack and push the concrete value that was returned.
            ClearStack(-1);
            PushConcrete(value);
        }
    }


    void SymbolicInterpreter::Branch(id_t id, branch_id_t bid, bool pred_value) {
        FILE *tr;
        //pid_t tid = syscall(__NR_gettid);
        tr = fopen("trace.txt","a");
        IFDEBUG(fprintf(tr, "branch %d %d\n", bid, pred_value));
//        std::cout << "stack size: " << stack_.size() << std::endl;

        assert(stack_.size() == 1);
        stack_.pop_back();

        if (pred_ && !pred_value) {
            pred_->Negate();
        }

        if(pred_)
            DumpPCFromBranch(pred_);
        ex_.mutable_path()->Push(bid, pred_);
        pred_ = NULL;
        IFDEBUG(DumpMemory());
    }


    value_t SymbolicInterpreter::NewInputValue(type_t type, addr_t addr, value_t v) {
        //ex_.mutable_inputs()->push_back(v);
        this->NewInputTemp(type, addr,v);
    }

    value_t SymbolicInterpreter::NewInputTemp(type_t type, addr_t addr, value_t val) {
        static unsigned int ghatiya = 0;
        num_inputs_ = ghatiya;
        mem_[addr] = new SymbolicExpr(1, num_inputs_);
        ex_.mutable_vars()->insert(make_pair(num_inputs_ ,type));

        value_t ret = val;
        if (num_inputs_ < ex_.inputs().size()) {
            ret = ex_.inputs()[num_inputs_];
        } else {
            // New inputs are initially zero.  (Could randomize instead.)
            ex_.mutable_inputs()->push_back(val);
        }

        num_inputs_ ++;
        ghatiya++;
        return ret;
    }

    value_t SymbolicInterpreter::NewInput(type_t type, addr_t addr) {
        static unsigned int ghatiya = 0;
        num_inputs_ = ghatiya;
        mem_[addr] = new SymbolicExpr(1, num_inputs_);
        ex_.mutable_vars()->insert(make_pair(num_inputs_ ,type));

        value_t ret = 0;
        if (num_inputs_ < ex_.inputs().size()) {
            ret = ex_.inputs()[num_inputs_];
        } else {
            // New inputs are initially zero.  (Could randomize instead.)
            ex_.mutable_inputs()->push_back(0);
        }

        num_inputs_ ++;
        ghatiya++;
        //fprintf(stderr,"New Input = %u\n",num_inputs_);
        return ret;
    }


    void SymbolicInterpreter::PushConcrete(value_t value) {
        PushSymbolic(NULL, value);
    }


    void SymbolicInterpreter::PushSymbolic(SymbolicExpr* expr, value_t value) {
        stack_.push_back(StackElem());
        StackElem& se = stack_.back();
        se.expr = expr;
        se.concrete = value;
    }


    void SymbolicInterpreter::ClearPredicateRegister() {
        delete pred_;
        pred_ = NULL;
    }

    void SymbolicInterpreter::CreateVarMap(addr_t addr, string* name, int tp, string* trigger) {
        //map<addr_t,string*>::const_iterator it = names_.find(addr);
        //if (it != names_.end())
        //    names_.erase(addr);
        //printf("name = %s, addr = %lu\n",name,addr);
        names_[addr] = name;
        names_typs_[addr] = tp;
        names_trigger_[addr] = trigger;
    }

    int foo(int state_id){
        FILE *f = fopen("state_id","w");
        fprintf(f,"%d\n",state_id-1);
        fclose(f);
    }

    int  SymbolicInterpreter::GetTimeStamp() {//aakanksha

        state_id++;
        foo(state_id);
        return state_id;

    }


    void  SymbolicInterpreter::PrintInput(char *name, int val) {//aakanksha

        FILE *tr;
        tr = fopen("trace.txt","a");
        fprintf(tr,"INPUT-%s,%d,%d\n",name,state_id,val);
        fclose(tr);

    }

    //void SymbolicInterpreter::ApplyLogState(int x) {
    void SymbolicInterpreter::ApplyLogState(int x, int r_w, int line, char *varname=NULL, int val=0 ,int *a=NULL) {//aakanksha
        FILE *tr;
        tr = fopen("trace.txt","a");
        
        FILE *observer;
        observer = fopen("observer.txt","a");


        //if (x == 100)
        //{
            //observer = fopen("observer.txt","w");
            //fprintf(observer,"OBSERVER\n");
        //}

        map<addr_t,string*>::const_iterator it;
        
        
        if (x == 100)
        {

            int q = -1;
            fprintf(observer, "\nLocation(State):%d,%d,%d,%d\n", q, state_id++,r_w,line);//aakanksha //YES
        }
        else
        {
            fprintf(tr, "\nLocation(State):%d,%d,%d,%d\n", x, state_id++,r_w,line);//aakanksha //YES
        }


        foo(state_id);
        for (ConstMemIt i = mem_.begin(); i != mem_.end(); ++i) {
            string s;
            it = names_.find(i->first);
            if (it == names_.end())
                continue;
            // c = it->second->c_str();
            s.clear();
            int tp = names_typs_.find(i->first)->second;
            string* trg = names_trigger_.find(i->first)->second;
            i->second->AppendToString(&s, tp);
            const char *c = "??";
            c = it->second->c_str();//change char pointer into string.

            fprintf(tr,"" );
            if (tp == 'i') // Integer
            {
                if (varname == NULL)
                {
                    if (x==100)
                    {
                        fprintf(observer, "%lu<%s>: %s [%d] {%s}\n", a, c, s.c_str(), *(int*)(i->first), trg->c_str());
                    }
                    //fprintf(tr,"hi how are you .1......... ....... ........... %lu \n",i->first);
                    else
                    {

                        fprintf(tr, "%lu<%s>: %s [%d] {%s}\n", a, c, s.c_str(), *(int*)(i->first), trg->c_str());
                    }

                }
            }
            else if (tp == 'c')
                if (x == 100){observer,(tr, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(char*)(i->first), trg->c_str());}
                else{fprintf(tr, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(char*)(i->first), trg->c_str());}
            else
            { 
                //fprintf(tr,"hi how are you 2.......... ....... ........... %lu \n",i->first);
                if (x == 100){fprintf(observer, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(int*)(i->first), trg->c_str());}
                else{fprintf(tr, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(int*)(i->first), trg->c_str());}
            }
        }

        // Log the concrete values --- that is those missing in the mem_ map
        for (map<addr_t,string*>::const_iterator i = names_.begin(); i != names_.end(); ++i)
        {

            int tp = names_typs_.find(i->first)->second;
            string* trg = names_trigger_.find(i->first)->second;
            //fprintf("%s",i->second->coeff);
            if (mem_.find(i->first) == mem_.end())
#ifdef PRINT_FOR_TOOL
                if (tp == 105) // Integer
                {
                    if (varname == NULL){
                        //fprintf(tr,"hi how are you 3.......... ....... ........... %lu \n",i->first);

                        if(x==100){fprintf(observer, "%lu<%s>: {const=%d} [%d] {%s}\n",a, i->second->c_str(), *(int*)(i->first) , *(int*)(i->first), trg->c_str());}
                        else{fprintf(tr, "%lu<%s>: {const=%d} [%d] {%s}\n",a, i->second->c_str(), *(int*)(i->first) , *(int*)(i->first), trg->c_str());}
                    }

                    else{
                        //fprintf(tr,"hi how are you 5........ ....... ........... %lu \n",i->first);
                        if(x == 100){fprintf(observer, "%lu<%s>:{const=%d} {%s}\n", a, varname, val,trg->c_str()) ;}
                        else{fprintf(tr, "%lu<%s>:{const=%d} {%s}\n", a, varname, val,trg->c_str()) ;}
                    }
                }
                else if (tp == 'c') // Integer
                    if (x == 100){fprintf(observer, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(char*)(i->first) , *(char*)(i->first), trg->c_str());}
                    else{fprintf(tr, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(char*)(i->first) , *(char*)(i->first), trg->c_str());}
                else
                {
                    //fprintf(tr,"hi how are you 4.......... ....... ........... %lu \n",i->first);
                    if(x == 100){fprintf(observer, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(int*)(i->first) , *(int*)(i->first), trg->c_str());}
                    else{fprintf(tr, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(int*)(i->first) , *(int*)(i->first), trg->c_str());}
                }
#else
            //fprintf(tr,"hi how are you 6.......... ....... ........... %lu \n",i->first);
            if(x == 100){fprintf(observer, "%lu<%s>: %s [%d]\n", a, i->second->c_str(), "concrete" , *(int*)(i->first));}
            else{fprintf(tr, "%lu<%s>: %s [%d]\n", a, i->second->c_str(), "concrete" , *(int*)(i->first));}
#endif
        }


        if (x == 100){fprintf(observer, "%s\n", "END");}
        else{fprintf(tr, "%s\n", "END");}

        ClearAllMaps();
        fclose(tr);
        if (x == 100)
            fclose(observer);

        // stack is the computation stack---not needed for us
        /*
           for (size_t i = 0; i < stack_.size(); i++) {
           string s;
           if (stack_[i].expr) {
           stack_[i].expr->AppendToString(&s);
           } else if ((i == stack_.size() - 1) && pred_) {
           pred_->AppendToString(&s);
           }
           it = names_.find(i);
           const char *c = "??";
           if (it != names_.end())
           c = it->second->c_str();
           fprintf(stderr, "s%d <%s>: %lld [ %s ]\n", i, c, stack_[i].concrete, s.c_str());
           }*/
    }


    /* void SymbolicInterpreter::ApplyLogState_1(int x) {
       map<addr_t,string*>::const_iterator it;
       fprintf(stderr, "\nLocation(State): %d, %d\n", x, state_id++);
       for (ConstMemIt i = mem_.begin(); i != mem_.end(); ++i) {
       string s;
       it = names_.find(i->first);
       if (it == names_.end())
       continue;
    // c = it->second->c_str();
    s.clear();
    int tp = names_typs_.find(i->first)->second;
    string* trg = names_trigger_.find(i->first)->second;
    i->second->AppendToString(&s, tp);
    const char *c = "??";
    c = it->second->c_str();
    if (tp == 'i') // Integer
    fprintf(stderr, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(int*)(i->first), trg->c_str());
    else if (tp == 'c')
    fprintf(stderr, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(char*)(i->first), trg->c_str());
    else
    fprintf(stderr, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(int*)(i->first), trg->c_str());
    }

    // Log the concrete values --- that is those missing in the mem_ map
    for (map<addr_t,string*>::const_iterator i = names_.begin(); i != names_.end(); ++i)
    {
    int tp = names_typs_.find(i->first)->second;
    string* trg = names_trigger_.find(i->first)->second;
    if (mem_.find(i->first) == mem_.end())
#ifdef PRINT_FOR_TOOL
if (tp == 'i') // Integer
fprintf(stderr, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(int*)(i->first) , *(int*)(i->first), trg->c_str());
else if (tp == 'c') // Integer
fprintf(stderr, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(char*)(i->first) , *(char*)(i->first), trg->c_str());
else
fprintf(stderr, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(int*)(i->first) , *(int*)(i->first), trg->c_str());
#else
fprintf(stderr, "%lu<%s>: %s [%d]\n", i->first, i->second->c_str(), "concrete" , *(int*)(i->first));
#endif
}

fprintf(stderr, "%s\n", "END");

ClearAllMaps();

// stack is the computation stack---not needed for us
//comment start
for (size_t i = 0; i < stack_.size(); i++) {
string s;
if (stack_[i].expr) {
stack_[i].expr->AppendToString(&s);
} else if ((i == stack_.size() - 1) && pred_) {
pred_->AppendToString(&s);
}
it = names_.find(i);
const char *c = "??";
if (it != names_.end())
c = it->second->c_str();
fprintf(stderr, "s%d <%s>: %lld [ %s ]\n", i, c, stack_[i].concrete, s.c_str());
}// comment end
}*/
void SymbolicInterpreter::ApplyLogState_1(int x) {// aakanksha
    FILE *fp;
    static int id = 0;
    fp = fopen("local_var.txt","a");
    map<addr_t,string*>::const_iterator it;
    fprintf(fp, "\nLocation(State): %d, %d\n", x, id++);
    for (ConstMemIt i = mem_.begin(); i != mem_.end(); ++i) {
        string s;
        it = names_.find(i->first);
        if (it == names_.end())
            continue;
        // c = it->second->c_str();
        s.clear();
        int tp = names_typs_.find(i->first)->second;
        string* trg = names_trigger_.find(i->first)->second;
        i->second->AppendToString(&s, tp);
        const char *c = "??";
        c = it->second->c_str();
        if (tp == 'i') //Integer
            fprintf(fp, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(int*)(i->first), trg->c_str());
        else if (tp == 'c')
            fprintf(fp, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(char*)(i->first), trg->c_str());
        else
            fprintf(fp, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(int*)(i->first), trg->c_str());
    }

    // Log the concrete values --- that is those missing in the mem_ map
    for (map<addr_t,string*>::const_iterator i = names_.begin(); i != names_.end(); ++i)
    {
        int tp = names_typs_.find(i->first)->second;
        string* trg = names_trigger_.find(i->first)->second;
        if (mem_.find(i->first) == mem_.end())
#ifdef PRINT_FOR_TOOL
            if (tp == 'i') // Integer
                fprintf(fp, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(int*)(i->first) , *(int*)(i->first), trg->c_str());
            else if (tp == 'c') // Integer
                fprintf(fp, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(char*)(i->first) , *(char*)(i->first), trg->c_str());
            else
                fprintf(fp, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(int*)(i->first) , *(int*)(i->first), trg->c_str());
#else
        fprintf(fp, "%lu<%s>: %s [%d]\n", i->first, i->second->c_str(), "concrete" , *(int*)(i->first));
#endif
    }

    fprintf(fp, "%s\n", "END");
    fclose(fp);
    ClearAllMaps();

    // stack is the computation stack---not needed for us
}

void SymbolicInterpreter::ApplyLogState_gdb(int x) {// aakanksha
    FILE *fp;
    static int id = 0;
    fp = fopen("logged_var.txt","a");
    map<addr_t,string*>::const_iterator it;
    fprintf(fp, "\nLocation(State): %d, %d\n", x, id++);
    for (ConstMemIt i = mem_.begin(); i != mem_.end(); ++i) {
        string s;
        it = names_.find(i->first);
        if (it == names_.end())
            continue;
        // c = it->second->c_str();
        s.clear();
        int tp = names_typs_.find(i->first)->second;
        string* trg = names_trigger_.find(i->first)->second;
        i->second->AppendToString(&s, tp);
        const char *c = "??";
        c = it->second->c_str();



        if (tp == 'i') //Integer
            fprintf(fp, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(int*)(i->first), trg->c_str());
        else if (tp == 'c')
            fprintf(fp, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(char*)(i->first), trg->c_str());
        else
            fprintf(fp, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(int*)(i->first), trg->c_str());
    }

    // Log the concrete values --- that is those missing in the mem_ map
    for (map<addr_t,string*>::const_iterator i = names_.begin(); i != names_.end(); ++i)
    {
        int tp = names_typs_.find(i->first)->second;
        string* trg = names_trigger_.find(i->first)->second;
        if (mem_.find(i->first) == mem_.end())
#ifdef PRINT_FOR_TOOL
            if (tp == 'i') // Integer
                fprintf(fp, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(int*)(i->first) , *(int*)(i->first), trg->c_str());
            else if (tp == 'c') // Integer
                fprintf(fp, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(char*)(i->first) , *(char*)(i->first), trg->c_str());
            else
                fprintf(fp, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(int*)(i->first) , *(int*)(i->first), trg->c_str());
#else
        fprintf(fp, "%lu<%s>: %s [%d]\n", i->first, i->second->c_str(), "concrete" , *(int*)(i->first));
#endif
    }

    fprintf(fp, "%s\n", "END");
    fclose(fp);
    ClearAllMaps();

    // stack is the computation stack---not needed for us
}

//it dumps PC in trace.txt
void SymbolicInterpreter::ApplyLogPC(int x) {
    FILE *tr;
    tr = fopen("trace.txt","a");
    string s;
    fprintf(tr, "\nLocation(PC): %d\n", x);//NO
    const SymbolicExecution& ex = execution();
    const SymbolicPath& path = ex.path();
    const vector<SymbolicPred*> constraints = path.constraints();
    for (size_t i = 0; i < constraints.size(); i++) {
        s.clear();
        constraints[i]->AppendToString(&s);
        fprintf(tr, "%s\n", s.c_str());
    }

    fprintf(tr, "%s\n", "END");
    fclose(tr);
}

void SymbolicInterpreter::ApplyLogPC_gdb(int x) {
    FILE *dp,*dp1;
    //dp = fopen("dump.txt","a");
    dp1 = fopen("dump1.txt","w");
    string s;
    //fprintf(dp, "\nLocation(PC): %d, %d\n", x, state_id++);
    fprintf(dp1, "\nLocation(PC): %d\n", x);
    foo(state_id);
    const SymbolicExecution& ex = execution();
    const SymbolicPath& path = ex.path();
    const vector<SymbolicPred*> constraints = path.constraints();
    for (size_t i = 0; i < constraints.size(); i++) {
        s.clear();
        constraints[i]->AppendToString(&s);
        //fprintf(dp, "%s\n", s.c_str());
        fprintf(dp1, "%s\n", s.c_str());
    }

    //fprintf(dp, "%s\n", "END");
    fprintf(dp1, "%s\n", "END");
    //fclose(dp);
    fclose(dp1);
}

//this functions dumps the thread id with the condition from the branch
//Created for MPI_verify

//This method returns the next operator/operand by parsing the string.
std::string getNext(std::string b, int *indx){
	int i = 0;
	char a[b.size()];
	while(*indx < b.size() && b[*indx] != ' ' && b[*indx] != ')')
			a[i++] = b[(*indx)++];
	a[i] = '\0';
	return a;
}

//This method parses the PC and simplifies
std::string simplifyBranchPC(std::string str){
	std::string result= "";
	std::stack<std::string> st;
	int indx = 0;
	while(indx < str.size()){
		//std::string first;
		if(str[indx] == ' '){
			indx++;
			continue;
		}
		else if(str[indx] == '('){
			indx++;
			std::string first = getNext(str, &indx);
			st.push(first);
		}
		else if(str[indx] == ')'){
			assert(st.size() >= 3);
			std::string three = st.top();
			st.pop();
			std::string two = st.top();
			st.pop();
			std::string one = st.top();
			st.pop();
			result = two + "_" + one + "_" + three;
			st.push(result);
			indx++;
		}
		else{
			std::string first = getNext(str, &indx);
			st.push(first);
		}
	}
	return result;
}

void SymbolicInterpreter::DumpPCFromBranch(SymbolicPred* pred_){
    int rank;
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
	string fname = "MpiTrace" + std::to_string(rank) + ".txt";
	FILE *pcFile = fopen(fname.c_str(), "a");
	//fseek(pcFile, 0, SEEK_END);
    string pcStr;
    pred_->AppendToString(&pcStr);
	printf("%s\n", pcStr.c_str());
	pcStr = simplifyBranchPC(pcStr); //@lavleshm
	printf("%s\n", pcStr.c_str());
    fprintf(pcFile, "branch(%d, %s)\n", rank, pcStr.c_str());
    fclose(pcFile);
	fsync(fileno(pcFile));
}

void SymbolicInterpreter::print(int tid, int r_w, int line, char* name, int val, int *addr){
    FILE *tr;
    tr = fopen("trace.txt","a");
    fprintf(tr, "\nLocation(State):%d,%d,%d,%d\n", tid, state_id++,r_w,line);//aakanksha
    fprintf(tr,"%lu<%s>: {const=%d} {True}\n",addr,name,val);
    fprintf(tr, "%s\n", "END");
    fclose(tr);

}
void SymbolicInterpreter::ApplyLogSpec(char *op,int *op1,int *op2) {
    //string s; 
    FILE *tr;
    tr = fopen("trace.txt","a");
    int x=1;
    fprintf(tr, "\nLocation(PC): %d, %d\n", x, state_id++);
    SymbolicExpr *val1, *val2;
    ConstMemIt it, it2; 
    string s;
    it = mem_.find((long unsigned int)op1);
    if (it == mem_.end())
    {
        SymbolicExpr s(*op1);
        val1 = &s;
    }
    else{
        val1 = it->second;
        it2 = mem_.find((long unsigned int)op2);
        if (it2 == mem_.end())
        {
            SymbolicExpr s(*op2);
            val2 = &s;
        }
        else{
            val2 = it2->second;
        }    
    }
    *val1 -= *val2 ;
    //int tp = names_typs_.find(op1)->second;
    //string* trg = names_trigger_.find(i->first)->second;
    //i->second->AppendToString(&s, tp);
    val1->AppendToString(&s, 'i');

    fprintf(tr,"(%s %s %d)\n" , op, s.c_str(), 0);
    fprintf(tr, "%s\n", "END");
    fclose(tr);
}

//Just add the size, type and head address of the symbolic array
void SymbolicInterpreter::addArrayElemMap(addr_t startAddr, int buffSize, type_t type){
    arrayElemMap_[startAddr] = std::make_pair(buffSize, type);
}

//pkalita
// return size of the type
int SymbolicInterpreter::getDataTypeSize(type_t type){
    if(type == types::U_CHAR)
        return sizeof(u_char);
    else if(type == types::CHAR)
        return sizeof(char);
    else if(type == types::U_SHORT)
        return sizeof(ushort);
    else if(type == types::SHORT)
        return sizeof(short );
    else if(type == types::U_INT)
        return sizeof(uint);
    else if(type == types::INT)
        return sizeof(int);

}

// pkalita
//checks if the addr belongs  to any symbolic array
// if true => returns array head
// else => return NULL
addr_t SymbolicInterpreter::isArrayElemPresent(addr_t addr){
    map<addr_t, std::pair<int, type_t>>::iterator itr;
    addr_t tempAddr = 0;
    int tempSize = 0;
    type_t tempType;
    int unitSize = 1; // to be used for size of memory according to type
    for(itr = arrayElemMap_.begin(); itr != arrayElemMap_.end(); itr++){
        tempAddr = itr->first;
        std::tie(tempSize, tempType) = itr->second;
        unitSize = getDataTypeSize(tempType);
        if(addr >= tempAddr && addr <= (tempAddr + tempSize * unitSize))
            return tempAddr;
    }
    return NULL;
}

// @lalveshm
bool SymbolicInterpreter::isAlreadySymbolic(addr_t addr){
	if(addrToSval.find(addr) != addrToSval.end()) return true;
	return false;
}

//@lavleshm
bool SymbolicInterpreter::setAddrToSval(addr_t addr, int value){
  if(addrToSval.find(addr) == addrToSval.end()){
  	addrToSval[addr] = value;
	return true;
  }
  return false;
}

//@lavleshm
int SymbolicInterpreter::getAddrToSval(addr_t addr){
//  assert(isAlreadySymbolic(addr));
  return addrToSval[addr];
}

}  // namespace crest


