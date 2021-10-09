import sys
from collections import defaultdict
from antlr4 import FileStream, CommonTokenStream, ParseTreeWalker
from MpiTraceLexer import MpiTraceLexer
from MpiTraceParser import MpiTraceParser
from MpiTraceListener import MpiTraceListener


class CspCreator:
    """Documentation for Channel
    Creates equivalent CSP methods
    """
    def __init__(self):
        self.channelData = ""
        # stores define constructs for routines
        self.defrData = ""
        # stores buffer variable declarations for send/recv calls
        self.bufData = "var threshold = 1024;\n"
        # dict form pid to routines
        self.routineData = defaultdict(lambda: "")
        self.routineList = defaultdict(lambda: list())
        # dict from request to pid
        # set by non-blocking calls and used by wait calls
        self.reqObjects = {}
        # dict from routine to associated channel
        # set by send calls and used by recv calls
        self.routineToChannels = defaultdict(lambda: list())
        # dict of list used as stack for storing parenthesis for
        # non-blocking calls
        self.waitStack = defaultdict(lambda: list())
        self.routineStack = defaultdict(lambda: list())
        # set to contain all the items that are already defined
        self.definedSet = set()
        # number of processes used
        self.num_procs = 1

    def fileWrite(self, fileName):
        '''
        Writes the csp encoding to an output file.
        '''
        fd = open(fileName, 'w')
        fd.write("//@@MpiTrace@@\n\n")
        fd.write(self.channelData)
        fd.write("\n")
        fd.write(self.bufData)
        fd.write("\n")
        fd.write(self.defrData)
        fd.write("\n")
        prog = []
        prog_ = [] 
        for key, value in self.routineList.items():
            parCount = 0
            pname = "P" + str(key) + "()"
            pdata = "P" + str(key) + "() = " + value[0]
            pname_ = "P" + str(key) + "_()"
            if value[0][4] == "I":
                pdata_ = "P" + str(key) + "_() = " + value[0] + " ||"
            else:
                pdata_ = "P" + str(key) + "_() = " + value[0] +" ;"
            for indx in range(1, len(value)):
                if value[indx] == "|| (" and ((indx < len(value)-1 \
                   and value[indx+1] != ")") or indx != len(value)-1):
                    pdata += " " + value[indx]
                    parCount += 1
                elif value[indx] == ")" and pdata[indx-1] != "|| (":
                    pdata += " " + value[indx]
                    parCount -= 1
                elif value[indx] == ";" and ((indx < len(value)-1 and \
                                              value[indx+1] != ")") or \
                                             indx != len(value)-1):
                    pdata += " " + value[indx]
                elif value[indx] != "|| (" and value[indx] != ";" and \
                     value[indx] != ")":
                    pdata += " " + value[indx]
                    if value[indx][4] == 'I':
                        pdata_ += value[indx] + " ||"
                    else:
                        pdata_ += value[indx] + " ;"
               # pdata += "P" + str(key) + "() = " + value  # + "Skip;\n"
            flag = False
            for vals in range(min(parCount, len(self.routineStack[key]))):
                pdata += ")"
                flag = True
            if flag:
                pdata += ";\n"
            else:
                pdata += ";\n"
            if pdata_[-1] == "|":
                pdata_ = pdata_[:-2] + ";"
            pdata_ +="\n"
            prog_.append(pname_)
            prog.append(pname)
            fd.write(pdata)
            fd.write(pdata_)
        progData = "Prog() = "
        for i in prog[:-1]:
            progData += i + " || "
        progData += prog[-1] + ";\n"
        fd.write(progData)
        progData_ = "Prog_() = "
        for i in prog_[:-1]:
            progData_ += i + " || "
        progData_ += prog_[-1] + ";\n"
        fd.write(progData_)
        propCheck = "#assert Prog() deadlockfree;\n"
        propCheck_ = "#assert Prog_() deadlockfree;\n"
        pref1 = "#assert Prog() refines Prog_();\n"
        pref2 = "#assert Prog_() refines Prog();\n"
        fd.write("\n")
        fd.write(propCheck)
        fd.write(propCheck_)
        fd.write(pref1)
        fd.write(pref2)
        fd.close()

    def defineRoutine(self, routineName, routineArgs, argString,
                      channelList=[]):
        '''
        Defines the equivalent csp routines( and stores the definition) for
        MPI routines and returns the cspRoutine name
        '''
        # Since MPI_Send's behaviour changes at run time based on the
        # buffer size, so modelled by taking buffer size into consideration
        cspRoutine = routineName + "_" + argString + "()"
        if cspRoutine in self.definedSet:
            return cspRoutine
        if routineName == "MPI_Send":
            bufVar = "SendBuf_" + argString
            if bufVar not in self.definedSet:
                self.bufData += "var " + bufVar + " = " + str(routineArgs[-2]) \
                + ";\n"
                self.definedSet.add(bufVar)
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            self.defrData += routineName + "_" + argString + "() = "
            self.defrData += "if( " + bufVar + " > threshold ) { " \
                + channelList[0] + "!" + bufVar + " -> Skip } else { " \
                + channelList[1] + "!" + bufVar + " -> Skip };\n"
        # for MPI_Send and MPI_Besnd, everything is same except the channel
        # size
        elif routineName == "MPI_Ssend":
            bufVar = "SendBuf_" + argString
            if bufVar not in self.definedSet:
                self.bufData += "var " + bufVar + " = " + str(routineArgs[-2]) \
                + ";\n"
                self.definedSet.add(bufVar)
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            self.defrData += routineName + "_" + argString + "() = "
            self.defrData += channelList[0] + "!" + bufVar + " -> Skip;\n"
        elif routineName == "MPI_Bsend":
            bufVar = "SendBuf_" + argString
            if bufVar not in self.definedSet:
                self.bufData += "var " + bufVar + " = " + str(routineArgs[-2]) \
                + ";\n"
                self.definedSet.add(bufVar)
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            self.defrData += routineName + "_" + argString + "() = "
            self.defrData += channelList[1] + "!" + bufVar + " -> Skip;\n"
        # standard non-blocking send routine, buffer size is taken into
        # cosideration for csp modelling
        elif routineName == "MPI_Isend":
            bufVar = "IsendBuf_" + argString
            if bufVar not in self.definedSet:
                self.bufData += "var " + bufVar + " = " + str(routineArgs[-2]) \
                + ";\n"
                self.definedSet.add(bufVar)
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            self.defrData += routineName + "_" + argString + "() = "
            reqObject = str(routineArgs[-3])
            if reqObject not in self.definedSet:
                self.bufData += "var " + reqObject + " = 0;\n"
                self.definedSet.add(reqObject)
            self.defrData += "if( " + bufVar + " > threshold ) { " \
                + channelList[0] + "!" + bufVar + " -> {" + reqObject \
                + " = 1} -> Skip } else { " + channelList[1] + "!" \
                + bufVar + " -> {" + reqObject + " = 1} -> Skip };\n"
        # For MPI_Issend and MPI_Ibsend everything is same except the channel
        # size.
        elif routineName == "MPI_Issend":
            bufVar = "IsendBuf_" + argString
            if bufVar not in self.definedSet:
                self.bufData += "var " + bufVar + " = " + str(routineArgs[-2]) \
                + ";\n"
                self.definedSet.add(bufVar)
            reqObject = str(routineArgs[-3])
            if reqObject not in self.definedSet:
                self.bufData += "var " + reqObject + " = 0;\n"
                self.definedSet.add(reqObject)
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            self.defrData += routineName + "_" + argString + "() = "
            self.defrData += channelList[0] + "?" + bufVar + " -> {" \
                + reqObject + " = 1} -> Skip;\n"
        elif routineName == "MPI_Ibsend":
            bufVar = "IsendBuf_" + argString
            if bufVar not in self.definedSet:
                self.bufData += "var " + bufVar + " = " + str(routineArgs[-2]) \
                + ";\n"
                self.definedSet.add(bufVar)
            reqObject = str(routineArgs[-3])
            if reqObject not in self.definedSet:
                self.bufData += "var " + reqObject + " = 0;\n"
                self.definedSet.add(reqObject)
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            self.defrData += routineName + "_" + argString + "() = "
            self.defrData += channelList[1] + "?" + bufVar + " {" \
                + reqObject + " = 1} -> Skip;\n"
        # Blocking recv routine
        elif routineName == "MPI_Recv":
            bufVar = "RecvBuf_" + argString
            if bufVar not in self.definedSet:
                self.bufData += "var " + bufVar + " = " + str(routineArgs[-2]) \
                + ";\n"
                self.definedSet.add(bufVar)
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            self.defrData += routineName + "_" + argString + "() = "
            if len(channelList) == 2:
                self.defrData += "if( " + bufVar + " > threshold ) { " \
                    + channelList[0] + "?[x <= " + bufVar \
                    + "]x -> Skip } \n\t\t\t\t\telse { " + channelList[1] \
                    + "?[x <= " + bufVar + "]x -> Skip [] " + channelList[0] \
                    + "?[x <= " + bufVar + "]x -> Skip };\n"
            else:               
                self.defrData += "if( " + bufVar + " > threshold ) { "
                for i in range(0, len(channelList), 2):
                    self.defrData +=  channelList[i] + "?[x <= " + bufVar \
                    + "]x -> Skip "
                    if i + 2 == len(channelList):
                        self.defrData += "}\n\t\t\t\t\t"
                    else:
                        self.defrData += "[]\n\t\t\t\t\t"
                self.defrData += "else { "
                for i in range(0, len(channelList), 2):
                    self.defrData +=  channelList[i+1] \
                    + "?[x <= " + bufVar + "]x -> Skip [] " + channelList[i] \
                    + "?[x <= " + bufVar + "]x -> Skip "
                    if i + 2 == len(channelList):
                        self.defrData += "};\n"
                    else:
                        self.defrData += "[]\n\t\t\t\t\t"
                    
        # Non-blocking recv routine.
        elif routineName == "MPI_Irecv":
            bufVar = "IrecvBuf_" + argString
            if bufVar not in self.definedSet:
                self.bufData += "var " + bufVar + " = " + str(routineArgs[-2]) \
                + ";\n"
                self.definedSet.add(bufVar)
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            self.defrData += routineName + "_" + argString + "() = "
            reqObject = str(routineArgs[-3])
            if reqObject not in self.definedSet:
                self.bufData += "var " + reqObject + " = 0;\n"
                self.definedSet.add(reqObject)
            if len(channelList) == 2:
                self.defrData += "if( " + bufVar + " > threshold ) { " \
                    + channelList[0] + "?[x <= " + bufVar + "]x -> {" \
                    + reqObject + " = 1} -> Skip } else { " + channelList[1] \
                    + "?[x <= " + bufVar + "]x -> {" + reqObject \
                    + " = 1} -> Skip [] " + channelList[0] + "?[x <= " \
                    + bufVar + "]x -> {" + reqObject + " = 1} -> Skip };\n"
            else:
                self.defrData += "if( " + bufVar + " > threshold ) { "
                for i in range(0, len(channelList), 2):
                    self.defrData += channelList[i] + "?[x <= " + bufVar \
                        + "]x -> {" + reqObject + " = 1} -> Skip "
                    if i + 2 == len(channelList):
                        self.defrData += "}\n\t\t\t\t\t"
                    else:
                        self.defrData += "[]\n\t\t\t\t\t"
                self.defrData += "else { "
                for i in range(0, len(channelList), 2):
                    self.defrData += channelList[i+1] + "?[x <= " + bufVar \
                    + "]x -> {" + reqObject + " = 1} -> Skip [] " \
                    + channelList[i] + "?[x <= " + bufVar + "]x -> {" \
                    + reqObject + " = 1} -> Skip "
                    if i + 2 == len(channelList):
                        self.defrData += "};\n"
                    else:
                        self.defrData += "[]\n\t\t\t\t\t"
                
        # Completion routines for non-blocking routine
        elif routineName == "MPI_Wait":
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            self.defrData += routineName + "_" + argString + "() = "
            reqObject = str(routineArgs[-2])
            self.defrData +=  "[" + reqObject + " == 1]" + cspRoutine[4:-2] \
                + " -> Skip;\n"
        elif routineName == "MPI_Waitany":
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            self.defrData += routineName + "_" + argString + "() = "
            # generating all the request objects in below for loop
            reqObjectInt = int("0" + routineArgs[1], 16)
            count = int(routineArgs[2])
            for i in range(count):
                reqObject = str(hex(reqObjectInt + i*4))
                # have to remove 0 from starting of reqObject
                if i == count - 1:
                    self.defrData += "[" + reqObject[1:] + " == 1]" \
                        + cspRoutine[4:-2] + " -> Skip "
                else:
                    self.defrData += "[" + reqObject[1:] + " == 1]" \
                        + cspRoutine[4:-2]+ " -> Skip [] "
                # a little formatting
                if i > 0 and i % 3 == 0:
                    self.defrData += "\n\t\t\t\t\t"
            self.defrData += ";\n"
        elif routineName == "MPI_Barrier":
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            commVar = "c" + str(routineArgs[1])
            if commVar not in self.definedSet:
                self.bufData += "var " + commVar + " = 0;\n"
                self.definedSet.add(commVar)
            self.defrData += routineName + "_" + argString + "() = "
            self.defrData += "{" + commVar + "++} -> ([" + commVar \
                + " > 0 && " + commVar + " %  num_procs == 0]Skip);\n"
        # make an entry that this routine is already created
        self.definedSet.add(cspRoutine)
        return cspRoutine

    def doChannelCreation(self, routineName, candidateArgs, argString):
        '''
        Handles channel creation logic for send routines
        '''
        # if already created just return channelList
        candidateArgsLocal = candidateArgs
        if self.isChannel(argString):
            return self.getcEntry(argString)
        # if channels are not yet created and a recv routine is
        # called then flip the 0th and f1st args and create channels
        if routineName == "MPI_Irecv" or routineName == "MPI_Recv":
            ch0 = self.createChannel(argString+"_s", candidateArgsLocal[1], 0)
            ch1 = self.createChannel(argString, candidateArgsLocal[1], 1)
        else:
            # for send routines create channels
            ch0 = self.createChannel(argString+"_s", candidateArgsLocal[0], 0)
            ch1 = self.createChannel(argString, candidateArgsLocal[0], 1)
        self.setcEntry(argString, [ch0, ch1])
        return self.getcEntry(argString)

    def createChannel(self, argString, pid, size):
        '''
        Helper method
        Creates a new channel
        '''
        channelName = "ch_" + argString
        self.channelData += "channel " + channelName + " " + str(size) + ";\n"
        return channelName

    def isChannel(self, argString):
        '''
        Helper method
        Checks if channels are already created.
        '''
        if argString in self.routineToChannels.keys():
            return True
        else:
            return False

    def setcEntry(self, argString, channelList):
        '''
        Helper method
        Adds channel name to the dictionary of list of channels,
        used to avoid creating redundant channels
        '''
        for channel in channelList:
            self.routineToChannels[argString].append(channel)

    def getcEntry(self, argString):
        '''
        Helper method
        Returns list of channels for a routine if already present
        '''
        channelList = self.routineToChannels[argString]
        return channelList

    def getArgString(self, candidateArgs, flip):
        '''
        Returns argString used for cspRoutine name creation
        '''
        if flip:
            argString = str(candidateArgs[1]) + "_" + str(candidateArgs[0]) \
                + "_"
            argString += "_".join([str(arg) for arg in candidateArgs[2:]])
        else:
            argString = "_".join([str(arg) for arg in candidateArgs])
        return argString

    def handleParentheses(self, routineArgs):
        '''
        Helper method
        Handles the parenthesization when using the completion routines
        of some non-blocking routine. It requires that every non-blocking
        routine must have a completion routine.
        '''
        flag = False
        while len(self.routineStack[routineArgs[0]]) > 0 and \
            len(self.waitStack[routineArgs[0]]) > 0 and \
            self.routineStack[routineArgs[0]][-1] == \
            self.waitStack[routineArgs[0]][-1]:
            self.routineData[routineArgs[0]] += " )"
            self.routineList[routineArgs[0]].append(")")
            self.routineStack[routineArgs[0]].pop()
            self.waitStack[routineArgs[0]].pop()
            flag = True
        return flag
    def createRoutine(self, routineName, routineArgs, reqSet=set()):
        '''
        This method creates equivalent csp encoding for the underlying
        MPI routine calls.
        '''
        if routineName == "MPI_Comm_size":
            # gives the number of processes used for execution
            self.num_procs = int(routineArgs[0])
            if "num_procs" not in self.definedSet:
                self.bufData += "var num_procs = " + str(self.num_procs) \
                    + ";\n"
                self.definedSet.add("num_procs")
        elif routineName == "MPI_Send" or routineName == "MPI_Ssend" \
           or routineName == "MPI_Bsend":
            # candidateArgs reperesent the arguments used for creating
            # csp routineName
            candidateArgs = routineArgs[:-2]
            argString = self.getArgString(candidateArgs, False)
            channelList = self.doChannelCreation(routineName, candidateArgs,
                                                 argString)
            cspRoutine = self.defineRoutine(routineName, routineArgs,
                                            argString, channelList)
            self.routineData[routineArgs[0]] += cspRoutine + " ; "
            self.routineList[routineArgs[0]].append(cspRoutine)
            self.routineList[routineArgs[0]].append(";")
        elif routineName == "MPI_Isend" or routineName == "MPI_Issend" \
             or routineName == "MPI_Ibsend":
            candidateArgs = routineArgs[:-3]
            argString = self.getArgString(candidateArgs, False)
            channelList = self.doChannelCreation(routineName, candidateArgs,
                                                 argString)
            cspRoutine = self.defineRoutine(routineName, routineArgs,
                                            argString, channelList)
            self.routineData[routineArgs[0]] += cspRoutine + " || ( "
            self.routineList[routineArgs[0]].append(cspRoutine)
            self.routineList[routineArgs[0]].append("|| (")
            # self.reqObjects[routineArgs[-3]] = routineArgs[0]
            self.routineStack[routineArgs[0]].append(routineArgs[-3])
        elif routineName == "MPI_Recv":
            candidateArgs = routineArgs[:-2]
            argString = self.getArgString(candidateArgs, False)
            argStringF = self.getArgString(candidateArgs, True)
            # channelList = ["ch_" + argStringF + "_s", "ch_" + argStringF]
            channelList = []
            if candidateArgs[1] == "A":
                # In case of MPI_ANY_SOURCE add more channels to then
                # channel list
                for src in range(0, self.num_procs):
                    candidateArgs[1] = src
                    argStringF = self.getArgString(candidateArgs, True)
                    channelList += self.doChannelCreation(routineName,
                                                          candidateArgs,
                                                          argStringF)
            else:
                channelList += self.doChannelCreation(routineName,
                                                      candidateArgs,
                                                      argStringF)
            cspRoutine = self.defineRoutine(routineName, routineArgs,
                                            argString, channelList)
            self.routineData[routineArgs[0]] += cspRoutine + " ; "
            self.routineList[routineArgs[0]].append(cspRoutine)
            self.routineList[routineArgs[0]].append(";")
        elif routineName == "MPI_Irecv":
            candidateArgs = routineArgs[:-3]
            argString = self.getArgString(candidateArgs, False)
            argStringF = self.getArgString(candidateArgs, True)
            # channelList = ["ch_" + argStringF + "_s", "ch_" + argStringF]
            channelList = []
            if candidateArgs[1] == "A":
                # In case of MPI_ANY_SOURCE add more channels to then
                # channel list
                for src in range(0, self.num_procs):
                    candidateArgs[1] = src
                    argStringF = self.getArgString(candidateArgs, True)
                    channelList += self.doChannelCreation(routineName,
                                                          candidateArgs,
                                                          argStringF)
            else:
                channelList += self.doChannelCreation(routineName,
                                                      candidateArgs,
                                                      argStringF)
            cspRoutine = self.defineRoutine(routineName, routineArgs,
                                            argString, channelList)
            self.routineData[routineArgs[0]] += cspRoutine + " || ( "
            self.routineList[routineArgs[0]].append(cspRoutine)
            self.routineList[routineArgs[0]].append("|| (")
            # self.reqObjects[routineArgs[-3]] = routineArgs[0]
            self.routineStack[routineArgs[0]].append(routineArgs[-3])
        elif routineName == "MPI_Wait":
            # It includes logic for parenthesis matching for the non-blocking
            # calls' definition. It requires that for every non-blocking
            # routine completion call must be present.
            candidateArgs = routineArgs[:-1]
            argString = self.getArgString(candidateArgs, False)
            cspRoutine = self.defineRoutine(routineName, routineArgs,
                                            argString)
            self.routineData[routineArgs[0]] += cspRoutine
            self.routineList[routineArgs[0]].append(cspRoutine)
            self.waitStack[routineArgs[0]].append(routineArgs[-2])
            flag = self.handleParentheses(routineArgs)
            self.routineData[routineArgs[0]] += "; "
            self.routineList[routineArgs[0]].append(";")
        elif routineName == "MPI_Waitany":
            # completes any of the given send/recv requests
            candidateArgs = routineArgs[:-1]
            argString = self.getArgString(candidateArgs, False)
            cspRoutine = self.defineRoutine(routineName, routineArgs,
                                            argString)
            self.routineData[routineArgs[0]] += cspRoutine
            self.routineList[routineArgs[0]].append(cspRoutine)
            flag = False
            # use parenthesization if any of the associated request
            # objects is at routineStack top
            if self.routineStack[routineArgs[0]][-1] in reqSet:
                curReq = self.routineStack[routineArgs[0]][-1]
                self.waitStack[routineArgs[0]].append(curReq)
                flag = self.handleParentheses(routineArgs)
            self.routineData[routineArgs[0]] += "; "
            self.routineList[routineArgs[0]].append(";")
        elif routineName == "MPI_Barrier":
            # implements barrier collective routine
            candidateArgs = routineArgs[:-1]
            argString = self.getArgString(candidateArgs, False)
            cspRoutine = self.defineRoutine(routineName, routineArgs,
                                            argString)
            self.routineData[routineArgs[0]] += cspRoutine + ";"
            self.routineList[routineArgs[0]].append(cspRoutine)
            self.routineList[routineArgs[0]].append(";")


class TraceListener(MpiTraceListener, CspCreator):
    '''
    Custom Listener implementation
    '''
    def enterRoutine(self, ctx):
        routineName = str(ctx.ID())

        # list of arguments of a routine
        args = ctx.getChild(2).getText().split(",")
        # represents the list of request objects associated with
        # multirequest completion routines
        reqSet = set()
        # For multirequest completion routines we record only first request
        # along with the count of request and genereate all the request objects
        # here using pointer addition.
        if routineName == "MPI_Waitany":
            count = int(args[-2])
            request = int(args[-3], 16)
            for i in range(count):
                reqSet.add(str(hex(request + i*4))[1:])
        args = ["C" if x[0:2] == "-3" else x for x in args]
        args = ["A" if x[0] == "-" else x for x in args]
        # Removing 0 from starting of a request objects so as it could directly
        # be used as a name of an event in the csp encoding
        routineArgs = [x[1:] if x[0:2] == "0x" else x for x in args]
        self.createRoutine(routineName, routineArgs, reqSet)


def main(argv):
    input_stream = FileStream(argv[1])
    op_file = "../test/output.csp"
    lexer = MpiTraceLexer(input_stream)
    stream = CommonTokenStream(lexer)
    parser = MpiTraceParser(stream)
    tree = parser.trace()
    printer = TraceListener()
    walker = ParseTreeWalker()
    walker.walk(printer, tree)
    printer.fileWrite(op_file)


if __name__ == '__main__':
    main(sys.argv)
