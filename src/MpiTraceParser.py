# Generated from MpiTrace.g4 by ANTLR 4.9.1
# encoding: utf-8
from antlr4 import *
from io import StringIO
import sys
if sys.version_info[1] > 5:
	from typing import TextIO
else:
	from typing.io import TextIO


def serializedATN():
    with StringIO() as buf:
        buf.write("\3\u608b\ua72a\u8133\ub9ed\u417c\u3be7\u7786\u5964\3\13")
        buf.write("A\4\2\t\2\4\3\t\3\4\4\t\4\4\5\t\5\4\6\t\6\4\7\t\7\3\2")
        buf.write("\3\2\5\2\21\n\2\3\2\3\2\3\2\5\2\26\n\2\3\2\7\2\31\n\2")
        buf.write("\f\2\16\2\34\13\2\3\3\3\3\3\3\3\3\3\3\3\4\3\4\3\4\7\4")
        buf.write("&\n\4\f\4\16\4)\13\4\3\5\3\5\5\5-\n\5\3\6\7\6\60\n\6\f")
        buf.write("\6\16\6\63\13\6\3\6\3\6\7\6\67\n\6\f\6\16\6:\13\6\3\7")
        buf.write("\6\7=\n\7\r\7\16\7>\3\7\2\2\b\2\4\6\b\n\f\2\3\3\2\t\n")
        buf.write("\2C\2\16\3\2\2\2\4\35\3\2\2\2\6\"\3\2\2\2\b,\3\2\2\2\n")
        buf.write("\61\3\2\2\2\f<\3\2\2\2\16\32\5\4\3\2\17\21\7\3\2\2\20")
        buf.write("\17\3\2\2\2\20\21\3\2\2\2\21\22\3\2\2\2\22\23\7\4\2\2")
        buf.write("\23\31\5\4\3\2\24\26\7\3\2\2\25\24\3\2\2\2\25\26\3\2\2")
        buf.write("\2\26\27\3\2\2\2\27\31\7\4\2\2\30\20\3\2\2\2\30\25\3\2")
        buf.write("\2\2\31\34\3\2\2\2\32\30\3\2\2\2\32\33\3\2\2\2\33\3\3")
        buf.write("\2\2\2\34\32\3\2\2\2\35\36\7\b\2\2\36\37\7\5\2\2\37 \5")
        buf.write("\6\4\2 !\7\6\2\2!\5\3\2\2\2\"\'\5\b\5\2#$\7\7\2\2$&\5")
        buf.write("\b\5\2%#\3\2\2\2&)\3\2\2\2\'%\3\2\2\2\'(\3\2\2\2(\7\3")
        buf.write("\2\2\2)\'\3\2\2\2*-\5\n\6\2+-\5\f\7\2,*\3\2\2\2,+\3\2")
        buf.write("\2\2-\t\3\2\2\2.\60\t\2\2\2/.\3\2\2\2\60\63\3\2\2\2\61")
        buf.write("/\3\2\2\2\61\62\3\2\2\2\62\64\3\2\2\2\63\61\3\2\2\2\64")
        buf.write("8\7\n\2\2\65\67\t\2\2\2\66\65\3\2\2\2\67:\3\2\2\28\66")
        buf.write("\3\2\2\289\3\2\2\29\13\3\2\2\2:8\3\2\2\2;=\7\t\2\2<;\3")
        buf.write("\2\2\2=>\3\2\2\2><\3\2\2\2>?\3\2\2\2?\r\3\2\2\2\13\20")
        buf.write("\25\30\32\',\618>")
        return buf.getvalue()


class MpiTraceParser ( Parser ):

    grammarFileName = "MpiTrace.g4"

    atn = ATNDeserializer().deserialize(serializedATN())

    decisionsToDFA = [ DFA(ds, i) for i, ds in enumerate(atn.decisionToState) ]

    sharedContextCache = PredictionContextCache()

    literalNames = [ "<INVALID>", "'\r'", "'\n'", "'('", "')'", "','" ]

    symbolicNames = [ "<INVALID>", "<INVALID>", "<INVALID>", "<INVALID>", 
                      "<INVALID>", "<INVALID>", "ID", "Digit", "Nondigit", 
                      "WS" ]

    RULE_trace = 0
    RULE_routine = 1
    RULE_argumentList = 2
    RULE_argument = 3
    RULE_string = 4
    RULE_number = 5

    ruleNames =  [ "trace", "routine", "argumentList", "argument", "string", 
                   "number" ]

    EOF = Token.EOF
    T__0=1
    T__1=2
    T__2=3
    T__3=4
    T__4=5
    ID=6
    Digit=7
    Nondigit=8
    WS=9

    def __init__(self, input:TokenStream, output:TextIO = sys.stdout):
        super().__init__(input, output)
        self.checkVersion("4.9.1")
        self._interp = ParserATNSimulator(self, self.atn, self.decisionsToDFA, self.sharedContextCache)
        self._predicates = None




    class TraceContext(ParserRuleContext):
        __slots__ = 'parser'

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser

        def routine(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(MpiTraceParser.RoutineContext)
            else:
                return self.getTypedRuleContext(MpiTraceParser.RoutineContext,i)


        def getRuleIndex(self):
            return MpiTraceParser.RULE_trace

        def enterRule(self, listener:ParseTreeListener):
            if hasattr( listener, "enterTrace" ):
                listener.enterTrace(self)

        def exitRule(self, listener:ParseTreeListener):
            if hasattr( listener, "exitTrace" ):
                listener.exitTrace(self)




    def trace(self):

        localctx = MpiTraceParser.TraceContext(self, self._ctx, self.state)
        self.enterRule(localctx, 0, self.RULE_trace)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 12
            self.routine()
            self.state = 24
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==MpiTraceParser.T__0 or _la==MpiTraceParser.T__1:
                self.state = 22
                self._errHandler.sync(self)
                la_ = self._interp.adaptivePredict(self._input,2,self._ctx)
                if la_ == 1:
                    self.state = 14
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)
                    if _la==MpiTraceParser.T__0:
                        self.state = 13
                        self.match(MpiTraceParser.T__0)


                    self.state = 16
                    self.match(MpiTraceParser.T__1)
                    self.state = 17
                    self.routine()
                    pass

                elif la_ == 2:
                    self.state = 19
                    self._errHandler.sync(self)
                    _la = self._input.LA(1)
                    if _la==MpiTraceParser.T__0:
                        self.state = 18
                        self.match(MpiTraceParser.T__0)


                    self.state = 21
                    self.match(MpiTraceParser.T__1)
                    pass


                self.state = 26
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class RoutineContext(ParserRuleContext):
        __slots__ = 'parser'

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser

        def ID(self):
            return self.getToken(MpiTraceParser.ID, 0)

        def argumentList(self):
            return self.getTypedRuleContext(MpiTraceParser.ArgumentListContext,0)


        def getRuleIndex(self):
            return MpiTraceParser.RULE_routine

        def enterRule(self, listener:ParseTreeListener):
            if hasattr( listener, "enterRoutine" ):
                listener.enterRoutine(self)

        def exitRule(self, listener:ParseTreeListener):
            if hasattr( listener, "exitRoutine" ):
                listener.exitRoutine(self)




    def routine(self):

        localctx = MpiTraceParser.RoutineContext(self, self._ctx, self.state)
        self.enterRule(localctx, 2, self.RULE_routine)
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 27
            self.match(MpiTraceParser.ID)
            self.state = 28
            self.match(MpiTraceParser.T__2)
            self.state = 29
            self.argumentList()
            self.state = 30
            self.match(MpiTraceParser.T__3)
        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class ArgumentListContext(ParserRuleContext):
        __slots__ = 'parser'

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser

        def argument(self, i:int=None):
            if i is None:
                return self.getTypedRuleContexts(MpiTraceParser.ArgumentContext)
            else:
                return self.getTypedRuleContext(MpiTraceParser.ArgumentContext,i)


        def getRuleIndex(self):
            return MpiTraceParser.RULE_argumentList

        def enterRule(self, listener:ParseTreeListener):
            if hasattr( listener, "enterArgumentList" ):
                listener.enterArgumentList(self)

        def exitRule(self, listener:ParseTreeListener):
            if hasattr( listener, "exitArgumentList" ):
                listener.exitArgumentList(self)




    def argumentList(self):

        localctx = MpiTraceParser.ArgumentListContext(self, self._ctx, self.state)
        self.enterRule(localctx, 4, self.RULE_argumentList)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 32
            self.argument()
            self.state = 37
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==MpiTraceParser.T__4:
                self.state = 33
                self.match(MpiTraceParser.T__4)
                self.state = 34
                self.argument()
                self.state = 39
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class ArgumentContext(ParserRuleContext):
        __slots__ = 'parser'

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser

        def string(self):
            return self.getTypedRuleContext(MpiTraceParser.StringContext,0)


        def number(self):
            return self.getTypedRuleContext(MpiTraceParser.NumberContext,0)


        def getRuleIndex(self):
            return MpiTraceParser.RULE_argument

        def enterRule(self, listener:ParseTreeListener):
            if hasattr( listener, "enterArgument" ):
                listener.enterArgument(self)

        def exitRule(self, listener:ParseTreeListener):
            if hasattr( listener, "exitArgument" ):
                listener.exitArgument(self)




    def argument(self):

        localctx = MpiTraceParser.ArgumentContext(self, self._ctx, self.state)
        self.enterRule(localctx, 6, self.RULE_argument)
        try:
            self.state = 42
            self._errHandler.sync(self)
            la_ = self._interp.adaptivePredict(self._input,5,self._ctx)
            if la_ == 1:
                self.enterOuterAlt(localctx, 1)
                self.state = 40
                self.string()
                pass

            elif la_ == 2:
                self.enterOuterAlt(localctx, 2)
                self.state = 41
                self.number()
                pass


        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class StringContext(ParserRuleContext):
        __slots__ = 'parser'

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser

        def Nondigit(self, i:int=None):
            if i is None:
                return self.getTokens(MpiTraceParser.Nondigit)
            else:
                return self.getToken(MpiTraceParser.Nondigit, i)

        def Digit(self, i:int=None):
            if i is None:
                return self.getTokens(MpiTraceParser.Digit)
            else:
                return self.getToken(MpiTraceParser.Digit, i)

        def getRuleIndex(self):
            return MpiTraceParser.RULE_string

        def enterRule(self, listener:ParseTreeListener):
            if hasattr( listener, "enterString" ):
                listener.enterString(self)

        def exitRule(self, listener:ParseTreeListener):
            if hasattr( listener, "exitString" ):
                listener.exitString(self)




    def string(self):

        localctx = MpiTraceParser.StringContext(self, self._ctx, self.state)
        self.enterRule(localctx, 8, self.RULE_string)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 47
            self._errHandler.sync(self)
            _alt = self._interp.adaptivePredict(self._input,6,self._ctx)
            while _alt!=2 and _alt!=ATN.INVALID_ALT_NUMBER:
                if _alt==1:
                    self.state = 44
                    _la = self._input.LA(1)
                    if not(_la==MpiTraceParser.Digit or _la==MpiTraceParser.Nondigit):
                        self._errHandler.recoverInline(self)
                    else:
                        self._errHandler.reportMatch(self)
                        self.consume() 
                self.state = 49
                self._errHandler.sync(self)
                _alt = self._interp.adaptivePredict(self._input,6,self._ctx)

            self.state = 50
            self.match(MpiTraceParser.Nondigit)
            self.state = 54
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while _la==MpiTraceParser.Digit or _la==MpiTraceParser.Nondigit:
                self.state = 51
                _la = self._input.LA(1)
                if not(_la==MpiTraceParser.Digit or _la==MpiTraceParser.Nondigit):
                    self._errHandler.recoverInline(self)
                else:
                    self._errHandler.reportMatch(self)
                    self.consume()
                self.state = 56
                self._errHandler.sync(self)
                _la = self._input.LA(1)

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx


    class NumberContext(ParserRuleContext):
        __slots__ = 'parser'

        def __init__(self, parser, parent:ParserRuleContext=None, invokingState:int=-1):
            super().__init__(parent, invokingState)
            self.parser = parser

        def Digit(self, i:int=None):
            if i is None:
                return self.getTokens(MpiTraceParser.Digit)
            else:
                return self.getToken(MpiTraceParser.Digit, i)

        def getRuleIndex(self):
            return MpiTraceParser.RULE_number

        def enterRule(self, listener:ParseTreeListener):
            if hasattr( listener, "enterNumber" ):
                listener.enterNumber(self)

        def exitRule(self, listener:ParseTreeListener):
            if hasattr( listener, "exitNumber" ):
                listener.exitNumber(self)




    def number(self):

        localctx = MpiTraceParser.NumberContext(self, self._ctx, self.state)
        self.enterRule(localctx, 10, self.RULE_number)
        self._la = 0 # Token type
        try:
            self.enterOuterAlt(localctx, 1)
            self.state = 58 
            self._errHandler.sync(self)
            _la = self._input.LA(1)
            while True:
                self.state = 57
                self.match(MpiTraceParser.Digit)
                self.state = 60 
                self._errHandler.sync(self)
                _la = self._input.LA(1)
                if not (_la==MpiTraceParser.Digit):
                    break

        except RecognitionException as re:
            localctx.exception = re
            self._errHandler.reportError(self, re)
            self._errHandler.recover(self, re)
        finally:
            self.exitRule()
        return localctx





