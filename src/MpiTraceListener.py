# Generated from MpiTrace.g4 by ANTLR 4.9.1
from antlr4 import *
if __name__ is not None and "." in __name__:
    from .MpiTraceParser import MpiTraceParser
else:
    from MpiTraceParser import MpiTraceParser

# This class defines a complete listener for a parse tree produced by MpiTraceParser.
class MpiTraceListener(ParseTreeListener):

    # Enter a parse tree produced by MpiTraceParser#trace.
    def enterTrace(self, ctx:MpiTraceParser.TraceContext):
        pass

    # Exit a parse tree produced by MpiTraceParser#trace.
    def exitTrace(self, ctx:MpiTraceParser.TraceContext):
        pass


    # Enter a parse tree produced by MpiTraceParser#routine.
    def enterRoutine(self, ctx:MpiTraceParser.RoutineContext):
        pass

    # Exit a parse tree produced by MpiTraceParser#routine.
    def exitRoutine(self, ctx:MpiTraceParser.RoutineContext):
        pass


    # Enter a parse tree produced by MpiTraceParser#argumentList.
    def enterArgumentList(self, ctx:MpiTraceParser.ArgumentListContext):
        pass

    # Exit a parse tree produced by MpiTraceParser#argumentList.
    def exitArgumentList(self, ctx:MpiTraceParser.ArgumentListContext):
        pass


    # Enter a parse tree produced by MpiTraceParser#argument.
    def enterArgument(self, ctx:MpiTraceParser.ArgumentContext):
        pass

    # Exit a parse tree produced by MpiTraceParser#argument.
    def exitArgument(self, ctx:MpiTraceParser.ArgumentContext):
        pass


    # Enter a parse tree produced by MpiTraceParser#string.
    def enterString(self, ctx:MpiTraceParser.StringContext):
        pass

    # Exit a parse tree produced by MpiTraceParser#string.
    def exitString(self, ctx:MpiTraceParser.StringContext):
        pass


    # Enter a parse tree produced by MpiTraceParser#number.
    def enterNumber(self, ctx:MpiTraceParser.NumberContext):
        pass

    # Exit a parse tree produced by MpiTraceParser#number.
    def exitNumber(self, ctx:MpiTraceParser.NumberContext):
        pass



del MpiTraceParser