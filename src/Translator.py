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
        self.channelCount = defaultdict(lambda: -1)
        # stores channel declarations
        self.channelData = ""
        # stores define constructs for routines
        self.defrData = ""
        # dict form pid to routines
        self.routineData = defaultdict(lambda: "")
        # dict from request to pid
        # set by non-blocking calls and used by wait calls
        self.reqObjects = {}
        # dict from routine to associated channel
        # set by send calls and used by recv calls
        self.routineToChannel = {}
        # dict of list used as stack for storing parenthesis for
        # non-blocking calls
        self.waitStack = defaultdict(lambda: list())
        self.routineStack = defaultdict(lambda: list())

    def fileWrite(self, fileName):
        fd = open(fileName, 'w')
        fd.write(self.channelData)
        fd.write("\n")
        fd.write(self.defrData)
        fd.write("\n")
        prog = []
        for key, value in self.routineData.items():
            pname = "P" + str(key) + "()"
            pdata = "P" + str(key) + "() = " + value# + "Skip;\n"
            flag = False
            for vals in self.routineStack[key]:
                pdata += ")"
                flag = True
            if flag:
                pdata += ";\n"
            else:
                pdata += "\n"
            prog.append(pname)
            fd.write(pdata)
        progData = "Prog() = "
        for i in prog[:-1]:
            progData += i + " || "
        progData += prog[-1] + ";\n"
        fd.write(progData)
        propCheck = "#assert Prog() deadlockfree;\n"
        fd.write("\n")
        fd.write(propCheck)
        fd.close()

    def createChannel(self, argString, pid, size):
        self.channelCount[pid] += 1
        channelName = "ch" + "_" + argString
        self.setcEntry(argString, channelName)
        self.channelData += "channel " + channelName + " " + str(size) + ";\n"
        return channelName

    def isChannel(self, argString):
        if argString in self.routineToChannel:
            return True
        else:
            return False

    def setcEntry(self, argString, channelName):
        self.routineToChannel[argString] = channelName

    def getcEntry(self, argString):
        channelName = self.routineToChannel[argString]
        self.routineToChannel.pop(argString)
        return channelName

    def completerName(self, routineArgs):
        cspRoutine = str(routineArgs[0])
        for rargs in routineArgs[1:]:
            cspRoutine += "_" + str(rargs)
        cspRoutine += "()"
        return cspRoutine

    def defineRoutine4(self, cspRoutine, matchedChannel, chOp, reqOp):
        self.defrData += cspRoutine + " = " + matchedChannel + chOp + " -> " \
            + reqOp + " -> Skip;\n"

    def defineRoutine3(self, cspRoutine, matchedChannel, chOp):
        self.defrData += cspRoutine + " = " + matchedChannel + chOp + \
            " -> Skip;\n"

    def defineRoutine2(self, cspRoutine, reqOp):
        self.defrData += cspRoutine + " = " + reqOp + " -> Skip;\n"

    def createRoutine(self, routineName, routineArgs):
        cspRoutine = ""
        if routineName == "MPI_Send":
            argString = '_'.join([str(arg) for arg in routineArgs])
            if self.isChannel(argString):
                channelName = self.getcEntry(argString)
            else:
                channelName = self.createChannel(argString, routineArgs[0], 0)
            cspRoutine += "Send_" + self.completerName(routineArgs)
            self.routineData[routineArgs[0]] += cspRoutine + " ; "
            self.defineRoutine3(cspRoutine, channelName, "!0")
        elif routineName == "MPI_Ssend":
            argString = '_'.join([str(arg) for arg in routineArgs])
            if self.isChannel(argString):
                channelName = self.getcEntry(argString)
            else:
                channelName = self.createChannel(argString, routineArgs[0], 0)
            cspRoutine += "Ssend_" + self.completerName(routineArgs)
            self.routineData[routineArgs[0]] += cspRoutine + " ; "
            self.defineRoutine3(cspRoutine, channelName, "!0")
        elif routineName == "MPI_Bsend":
            argString = '_'.join([str(arg) for arg in routineArgs])
            cspRoutine += "Bsend_" + self.completerName(routineArgs)
            if self.isChannel(argString):
                channelName = self.getcEntry(argString)
            else:
                channelName = self.createChannel(argString, routineArgs[0], 1)
            self.routineData[routineArgs[0]] += cspRoutine + " ; "
            self.defineRoutine3(cspRoutine, channelName, "!0")
        elif routineName == "MPI_Isend":
            argString = '_'.join([str(arg) for arg in routineArgs[:-1]])
            if self.isChannel(argString):
                channelName = self.getcEntry(argString)
            else:
                channelName = self.createChannel(argString, routineArgs[0], 0)
            cspRoutine += "Isend_" + self.completerName(routineArgs)
            self.routineData[routineArgs[0]] += cspRoutine + " || ( "
            self.reqObjects[routineArgs[-1]] = routineArgs[0]
            self.routineStack[routineArgs[0]].append(routineArgs[-1])
            self.defineRoutine4(cspRoutine, channelName, "!0", routineArgs[-1])
        elif routineName == "MPI_Recv":
            argString = str(routineArgs[1]) + "_" + str(
                routineArgs[0]) + "_" + str(routineArgs[2])
            cspRoutine += "Recv_" + self.completerName(routineArgs)
            self.routineData[routineArgs[0]] += cspRoutine + " ; "
            #if self.isChannel(argString):
            #    channelName = self.getcEntry(argString)
            #else:
            #    channelName = self.createChannel(argString, routineArgs[0], 0)
            self.defineRoutine3(cspRoutine, "ch" + "_" + argString, "?0")
        elif routineName == "MPI_Irecv":
            argString = str(routineArgs[1]) + "_" + str(
                routineArgs[0]) + "_" + str(routineArgs[2])
            cspRoutine += "Irecv_" + self.completerName(routineArgs)
            self.routineData[routineArgs[0]] += cspRoutine + " || ( "
            self.reqObjects[routineArgs[-1]] = routineArgs[0]
            self.routineStack[routineArgs[0]].append(routineArgs[-1])
            #if self.isChannel(argString):
            #    channelName = self.getcEntry(argString)
            #else:
            #    channelName = self.createChannel(argString, routineArgs[0], 0)
            self.defineRoutine4(cspRoutine, "ch" + "_" + argString, "?0", routineArgs[-1])
        elif routineName == "MPI_Wait":
            cspRoutine += "Wait_" + self.completerName(routineArgs)
            self.routineData[self.reqObjects[routineArgs[1]]] += cspRoutine
            flag1 = True
            self.waitStack[routineArgs[0]].append(routineArgs[-1])
            while len(self.routineStack[routineArgs[0]]) > 0 and len(self.waitStack[routineArgs[0]]) > 0 and self.routineStack[routineArgs[0]][-1] == self.waitStack[routineArgs[0]][-1]:
                flag1 = False
                self.routineData[self.reqObjects[
                routineArgs[1]]] += " )"
                self.routineStack[routineArgs[0]].pop()
                self.waitStack[routineArgs[0]].pop()
            self.routineData[self.reqObjects[
            routineArgs[1]]] += "; " 
            self.defineRoutine2(cspRoutine, routineArgs[-1])
        elif routineName == "MPI_Barrier":
            cspRoutine += "Barrier_" + self.completerName(routineArgs)
            self.routineData[self.reqObjects[
                routineArgs[1]]] + cspRoutine + " ; "
            self.defineRoutine2(cspRoutine, routineArgs[-1])


class TraceListener(MpiTraceListener, CspCreator):
    '''
    Custom Listener implementation
    '''
    def enterRoutine(self, ctx):
        routineName = str(ctx.ID())
        args = ctx.getChild(2)
        routineArgs = []
        for i in range(args.getChildCount()):
            childText = args.getChild(i).getText()
            if childText != ',':
                if childText[0:2] == "0x":
                    routineArgs.append(str(childText[1:]))
                else:
                    routineArgs.append(str(childText))
        #print(routineArgs[:-1])
        if routineName == "MPI_Wait" or routineName == "MPI_Barrier" or routineName == "MPI_Finalize":
            self.createRoutine(routineName, routineArgs[:-1])
        else:
            self.createRoutine(routineName, routineArgs[:-2])


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
