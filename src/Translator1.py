import sys
import hashlib
from collections import defaultdict
from antlr4 import FileStream, CommonTokenStream, ParseTreeWalker
from MpiTraceLexer import MpiTraceLexer
from MpiTraceParser import MpiTraceParser
from MpiTraceListener import MpiTraceListener


class CspCreator:
    """Documentation for Channel
    Creates equivalent CSP methods
    """
    def __init__(self, channelSetDict):
        # dict from pid to channels as collected in preprocessing
        self.channelSetDict = channelSetDict
        self.channelData = ""
        # stores define constructs for routines
        self.defrData = ""
        # dict from hash to cspRoutine name
        self.hashTocspRoutine = {}
        # dict from branch variables to associated recv buffers
        self.branchVarsTorbufs = {}
        # stores buffer variable declarations for send/recv calls
        self.bufData = "var threshold = 131072;\nvar exit_ = false;\n"
        # dict form pid to routines
        self.routineList = defaultdict(lambda: list())
        # dict from request to pid
        # set by non-blocking calls and used by wait calls
        self.reqObjects = {}
        # dict from routine to associated channel
        # set by send calls and used by recv calls
        self.routineToChannels = defaultdict(lambda: list())
        # set to contain all the items that are already defined
        self.definedSet = set()
        # number of processes used
        self.num_procs = 1

    def createFlagProcess(self, routineName, reqObject):
        # fpName = "Reached_" + routineName[4:-2] + "_" + reqObject + "()"
        # fpVar = "flag_" + routineName[4:-2] + "_" + reqObject
        fpName = "R_" + routineName[4:-2] + "_" + "()"
        fpVar = "F_" + routineName[4:-2]
        if fpName in self.definedSet:
            return [fpName, fpVar]
        self.bufData += "var " + fpVar + " = false;\n"
        self.defrData += fpName + " = " + "{" + fpVar \
            + " = true} -> Skip;\n"
        self.definedSet.add(fpName)
        return [fpName, fpVar]

    def fileWrite(self, fileName):
        '''
        Writes the csp encoding to an output file.
        '''
        fd = open(fileName, 'w')
        fd.write("//@@MpiTrace@@\n\n")
        fd.write(self.channelData)
        fd.write("\n")
        prog = []
        pdata = ""
        for key, value in self.routineList.items():
            pname = "P" + str(key) + "()"
            pdata += "P" + str(key) + "() = ("
            parallel_pdata = ""
            for indx in range(0, len(value)):
                if value[indx][:4] == "(if(":
                    if indx == 0:
                        pdata += value[indx]
                    else:
                        pdata += "; " + value[indx]
                elif value[indx][:4] != "MPI_":
                    continue
                elif value[indx][:5] == 'MPI_I':
                    fpList = self.createFlagProcess(value[indx],
                                                    value[indx + 1])
                    parallel_pdata += " || [" + fpList[1] + " == true]" \
                        + value[indx]
                    if indx == 0:
                        pdata += fpList[0]
                    else:
                        pdata += "; " + fpList[0]
                else:
                    if indx == 0:
                        pdata += value[indx]
                    else:
                        pdata += "; " + value[indx]
            pdata += parallel_pdata + ")[][exit_ == true]Skip"
            pdata += ";\n"
            prog.append(pname)
        # Write buffers and routines definition
        fd.write(self.bufData)
        fd.write("\n")
        fd.write(self.defrData)
        fd.write("\n")
        # Write pdata
        fd.write(pdata)
        progData = "Prog() = "
        for i in prog[:-1]:
            progData += i + " || "
        progData += prog[-1] + ";\n"
        fd.write(progData)
        fd.write("Prog1() = {threshold = 0} -> Prog();\n")
        propCheck = "#assert Prog() deadlockfree;\n"
        propCheck += "#assert Prog1() deadlockfree;\n"
        fd.write("\n")
        fd.write(propCheck)
        fd.close()

    def getHash(self, routineName, routineArgs, argString):
        '''
        Returns a hash signature of a routine. Used to prevent the 
        redefining of equivalent routines.
        '''
        List1 = [
            "MPI_Send", "MPI_Bsend", "MPI_Rsend", "MPI_Ssend", "MPI_Isend",
            "MPI_Ibsend", "MPI_Irsend", "MPI_Issend"
        ]
        List3 = ["MPI_Recv", "MPI_Irecv"]
        methodSign = ""
        if routineName in List1 or routineName in List3:
            if routineName in List1:
                methodSign = routineName + self.getArgString(routineArgs[:-1], \
                                                             False)
            else:
                methodSign = routineName + self.getArgString(routineArgs[:-1], \
                                                             True)
        else:
            methodSign = routineName + "_" + argString + "()"
        hash_methodSign = hashlib.md5(methodSign.encode())
        hashed_sign = hash_methodSign.hexdigest()
        return hashed_sign

    def handleBufferNames(self, bufVar, rank, size, loggedBufName):
        '''It handles the mapping of the recv buffer elements with the variable
        names in the branch statements'''
        prefix = "x_" + rank + "_"
        firstSuffix = int(loggedBufName[1:])
        # little redundancy check, but need to be worked more carefully
        if prefix + str(firstSuffix) in self.branchVarsTorbufs.keys():
            return
        for i in range(int(size)):
            newName = prefix + str(firstSuffix + i)
            self.branchVarsTorbufs[newName] = bufVar + "[" + str(i) + "]"

    def defineRoutine(self,
                      routineName,
                      routineArgs,
                      argString,
                      channelList=[]):
        '''
        Defines the equivalent csp routines( and stores the definition) for
        MPI routines and returns the cspRoutine name
        '''
        # Since MPI_Send's behaviour changes at run time based on the
        # buffer size, so modelled by taking buffer size into consideration
        hashed_sign = self.getHash(routineName, routineArgs, argString)
        if hashed_sign in self.hashTocspRoutine.keys():
            cspRoutine = self.hashTocspRoutine[hashed_sign]
            return cspRoutine
        # If a matching routined is not already defined
        List1 = ["MPI_Send", "MPI_Bsend", "MPI_Rsend", "MPI_Ssend", "MPI_Recv"]
        List2 = [
            "MPI_Isend", "MPI_Ibsend", "MPI_Irsend", "MPI_Issend", "MPI_Irecv"
        ]
        # Here the routine name contain name-me-dst/src-tag-line
        if routineName in List1:
            cspRoutine = routineName + "_" + argString + "_" + routineArgs[-1] \
                + "()"
        elif routineName in List2:
            cspRoutine = routineName + "_" + argString + "_" + routineArgs[3] \
                + "_" + routineArgs[-1] + "()"
        else:
            cspRoutine = routineName + "_" + argString + "()"
        collList = [
            "MPI_Bcast", "MPI_Reduce", "MPI_Gather", "MPI_Scatter",
            "MPI_Allgather", "MPI_Allreduce", "MPI_Alltoall", "MPI_Alltoallv"
        ]
        #if cspRoutine in self.definedSet:
        #    return cspRoutine
        # Here the specific routines are defined
        if routineName == "MPI_Send":
            # denotes the size to be sent in number of bytes
            originalSize = int(routineArgs[3]) * int(routineArgs[4])
            # subffer is the actual buffer to be sent, the last argument
            # is originalSize(to be cheked at recv side)
            # buflen denotes the length of the logged buffer
            # buflen = len(list(routineArgs[1:-1].split('_')))
            # Case when 0 elements are logged.
            if (len(routineArgs[-2]) <= 2):
                sbuffer = "[" + str(originalSize) + "]"
            # case when atleast one element is logged
            else:
                sbuffer = routineArgs[-2][:-1].replace('_', ', ') + ", " \
                    + str(originalSize) + "]"
            # bufVar is the name of buffer variable
            bufVar = "SendBuf_" + argString + "_" + routineArgs[-1]
            # Older logic need to be cheked
            if bufVar not in self.definedSet:
                self.bufData += "var " + bufVar + " = " + sbuffer + ";\n"
                self.definedSet.add(bufVar)
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            self.defrData += cspRoutine + " = " + cspRoutine[:-2] \
                + "_starts -> "
            self.defrData += "if( " + str(originalSize) + " > threshold ) { " \
                + channelList[0] + "!" + bufVar + " -> " + cspRoutine[:-2] \
                + "_ends -> Skip } else { " + channelList[1] + "!" + bufVar \
                + " -> " + cspRoutine[:-2] + "_ends -> Skip };\n"
        # for MPI_Send and MPI_Besnd, everything is same except the channel
        # size
        elif routineName == "MPI_Ssend":
            # denotes the size to be sent in number of bytes
            originalSize = int(routineArgs[3]) * int(routineArgs[4])
            # subffer is the actual buffer to be sent, the last argument
            # is originalSize(to be cheked at recv side)
            # buflen denotes the length of the logged buffer
            # buflen = len(list(routineArgs[1:-1].split('_')))
            if (len(routineArgs[-2]) <= 2):
                sbuffer = "[" + str(originalSize) + "]"
            else:
                sbuffer = routineArgs[-2][:-1].replace('_', ', ') + ", " \
                    + str(originalSize) + "]"
            # bufVar is the name of buffer variable
            bufVar = "SsendBuf_" + argString + "_" + routineArgs[-1]
            if bufVar not in self.definedSet:
                self.bufData += "var " + bufVar + " = " + sbuffer + ";\n"
                self.definedSet.add(bufVar)
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            self.defrData += cspRoutine + " = " + cspRoutine[:-2] \
                + "_starts -> "
            self.defrData += channelList[0] + "!" + bufVar + " -> " \
                + cspRoutine[:-2] + "_ends -> Skip;\n"
        elif routineName == "MPI_Bsend":
            # denotes the size to be sent in number of bytes
            originalSize = int(routineArgs[3]) * int(routineArgs[4])
            # subffer is the actual buffer to be sent, the last argument
            # is originalSize(to be cheked at recv side)
            # buflen denotes the length of the logged buffer
            # buflen = len(list(routineArgs[1:-1].split('_')))
            if (len(routineArgs[-2]) <= 2):
                sbuffer = "[" + str(originalSize) + "]"
            else:
                sbuffer = routineArgs[-2][:-1].replace('_', ', ') + ", " \
                    + str(originalSize) + "]"
            # bufVar is the name of buffer variable
            bufVar = "BsendBuf_" + argString + "_" + routineArgs[-1]
            if bufVar not in self.definedSet:
                self.bufData += "var " + bufVar + " = " + sbuffer + ";\n"
                self.definedSet.add(bufVar)
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            self.defrData += cspRoutine + " = " + cspRoutine[:-2] \
                + "_starts -> "
            self.defrData += channelList[1] + "!" + bufVar + " -> " \
                + cspRoutine[:-2] + "_ends -> Skip;\n"
        # standard non-blocking send routine, buffer size is taken into
        # cosideration for csp modelling
        elif routineName == "MPI_Isend":
            # denotes the size to be sent in number of bytes
            originalSize = int(routineArgs[4]) * int(routineArgs[5])
            # subffer is the actual buffer to be sent, the last argument
            # is originalSize(to be cheked at recv side)
            # buflen denotes the length of the logged buffer
            # buflen = len(list(routineArgs[1:-1].split('_')))
            if (len(routineArgs[-2]) <= 2):
                sbuffer = "[" + str(originalSize) + "]"
            else:
                sbuffer = routineArgs[-2][:-1].replace('_', ', ') + ", " \
                    + str(originalSize) + "]"
            # bufVar is the name of buffer variable
            bufVar = "IsendBuf_" + argString + "_" + routineArgs[-1]
            if bufVar not in self.definedSet:
                self.bufData += "var " + bufVar + " = " + sbuffer + ";\n"
                self.definedSet.add(bufVar)
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            self.defrData += cspRoutine + " = " + cspRoutine[:-2] \
                + "_starts -> "
            reqObject = str(routineArgs[3])
            if reqObject not in self.definedSet:
                self.bufData += "var " + reqObject + " = 0;\n"
                self.definedSet.add(reqObject)
            self.defrData += "if( " + str(originalSize) + " > threshold ) { " \
                + channelList[0] + "!" + bufVar + " -> {" + reqObject \
                + " = 1} -> " + cspRoutine[:-2] + "_ends -> Skip } else { " \
                + channelList[1] + "!" + bufVar + " -> {" + reqObject \
                + " = 1} -> " + cspRoutine[:-2] + "_ends -> Skip };\n"
        # For MPI_Issend and MPI_Ibsend everything is same except the channel
        # size.
        elif routineName == "MPI_Issend":
            # denotes the size to be sent in number of bytes
            originalSize = int(routineArgs[4]) * int(routineArgs[5])
            # subffer is the actual buffer to be sent, the last argument
            # is originalSize(to be cheked at recv side)
            # buflen denotes the length of the logged buffer
            # buflen = len(list(routineArgs[1:-1].split('_')))
            if (len(routineArgs[-2]) <= 2):
                sbuffer = "[" + str(originalSize) + "]"
            else:
                sbuffer = routineArgs[-2][:-1].replace('_', ', ') + ", " \
                    + str(originalSize) + "]"
            # bufVar is the name of buffer variable
            bufVar = "IssendBuf_" + argString + "_" + routineArgs[-1]
            if bufVar not in self.definedSet:
                self.bufData += "var " + bufVar + " = " + sbuffer + ";\n"
                self.definedSet.add(bufVar)
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            self.defrData += cspRoutine + " = " + cspRoutine[:-2] \
                + "_starts -> "
            reqObject = str(routineArgs[3])
            self.defrData += channelList[0] + "?" + bufVar + " -> {" \
                + reqObject + " = 1} -> " + cspRoutine[:-2] + "_ends -> Skip;\n"
        elif routineName == "MPI_Ibsend":
            # denotes the size to be sent in number of bytes
            originalSize = int(routineArgs[4]) * int(routineArgs[5])
            # subffer is the actual buffer to be sent, the last argument
            # is originalSize(to be cheked at recv side)
            # buflen denotes the length of the logged buffer
            # buflen = len(list(routineArgs[1:-1].split('_')))
            if (len(routineArgs[-2]) <= 2):
                sbuffer = "[" + str(originalSize) + "]"
            else:
                sbuffer = routineArgs[-2][:-1].replace('_', ', ') + ", " \
                    + str(originalSize) + "]"
            # bufVar is the name of buffer variable
            bufVar = "IbsendBuf_" + argString + "_" + routineArgs[-1]
            if bufVar not in self.definedSet:
                self.bufData += "var " + bufVar + " = " + sbuffer + ";\n"
                self.definedSet.add(bufVar)
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            self.defrData += cspRoutine + " = " + cspRoutine[:-2] \
                + "_starts -> "
            reqObject = str(routineArgs[3])
            self.defrData += channelList[1] + "?" + bufVar + " {" \
                + reqObject + " = 1} -> " + cspRoutine[:-2] + "_ends -> Skip;\n"
        # Blocking recv routine
        elif routineName == "MPI_Recv":
            # denotes the size to be sent in number of bytes
            originalSize = int(routineArgs[3]) * int(routineArgs[4])
            # here bufVar also contains the line number
            bufVar = "RecvBuf_" + argString + "_" + routineArgs[-1]
            # Only need to create buffers for the symbolic variables,
            # which can be at most equal to logged symbolic variables
            requiredBufSize = str(int(routineArgs[-3]) + 1)
            # Handling buffer names and provide mapping to the symbolic variable
            # names received in branch statements and the recv statements
            self.handleBufferNames(bufVar, routineArgs[0], requiredBufSize,
                                   routineArgs[-2])
            if bufVar not in self.definedSet:
                self.bufData += "var " + bufVar + "[" + requiredBufSize + \
                    "];\n"
                self.definedSet.add(bufVar)
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            # denotes the last index of the recv buffer
            lindx = routineArgs[-3]
            self.defrData += cspRoutine + " = " + cspRoutine[:-2] \
                + "_starts -> "
            if len(channelList) == 2:
                self.defrData += "if( " + str(originalSize) \
                    + " > threshold ) { " + channelList[0] + "?[x[" \
                    + lindx + "] <= " + str(originalSize) + "]x ->{" \
                    + bufVar + " = x} -> " + cspRoutine[:-2] \
                    + "_ends -> Skip }\n\t\t\t\t\telse { " \
                    + channelList[1] + "?[x[" + lindx + "] <= " \
                    + str(originalSize) + "]x ->{" + bufVar \
                    + " = x} -> " + cspRoutine[:-2] + "_ends -> Skip [] " \
                    + channelList[0] + "?[x[" + lindx + "] <= " \
                    + str(originalSize) + "]x ->{" + bufVar + " = x} -> " \
                    + cspRoutine[:-2] + "_ends -> Skip };\n"
            else:
                self.defrData += "if( " + str(
                    originalSize) + " > threshold ) { "
                for i in range(0, len(channelList), 2):
                    self.defrData += channelList[i] + "?[x[" + lindx \
                        + "] <= " + str(originalSize) + "]x ->{" + bufVar \
                        + " = x} -> " + cspRoutine[:-2] + "_ends -> Skip "
                    if i + 2 == len(channelList):
                        self.defrData += "}\n\t\t\t\t\t"
                    else:
                        self.defrData += "[]\n\t\t\t\t\t"
                self.defrData += "else { "
                for i in range(0, len(channelList), 2):
                    self.defrData += channelList[i+1] \
                    + "?[x[" + lindx + "] <= " + str(originalSize) + "]x ->{" \
                    + bufVar + " = x} -> " + cspRoutine[:-2] \
                    + "_ends -> Skip [] " + channelList[i] \
                    + "?[x[" + lindx + "] <= " + str(originalSize) + "]x ->{" \
                    + bufVar + " = x} -> " + cspRoutine[:-2] + "_ends -> Skip "
                    if i + 2 == len(channelList):
                        self.defrData += "};\n"
                    else:
                        self.defrData += "[]\n\t\t\t\t\t"
        # Non-blocking recv routine.
        elif routineName == "MPI_Irecv":
            # denotes the size to be sent in number of bytes
            originalSize = int(routineArgs[4]) * int(routineArgs[5])
            # here bufVar also contains the line number
            bufVar = "RecvBuf_" + argString + "_" + routineArgs[-1]
            # Only need to create buffers for the symbolic variables,
            # which can be at most equal to logged symbolic variables
            # + 1 is used to recv the buffersize
            requiredBufSize = str(int(routineArgs[-3]) + 1)
            # Handling buffer names and provide mapping to the symbolic variable
            # names received in branch statements and the recv statements
            self.handleBufferNames(bufVar, routineArgs[0], requiredBufSize,
                                   routineArgs[-2])
            if bufVar not in self.definedSet:
                self.bufData += "var " + bufVar + "[" + requiredBufSize + \
                    "];\n"
                self.definedSet.add(bufVar)
            # denotes the last index of the recv buffer
            lindx = routineArgs[-3]
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            self.defrData += cspRoutine + " = " + cspRoutine[:-2] \
                + "_starts -> "
            reqObject = str(routineArgs[3])
            if reqObject not in self.definedSet:
                self.bufData += "var " + reqObject + " = 0;\n"
                self.definedSet.add(reqObject)
            if len(channelList) == 2:
                self.defrData += "if( " + str(originalSize) \
                    + " > threshold ) { " + channelList[0] + "?[x[" \
                    + lindx + "] <= " + str(originalSize) + "]x ->{" \
                    + bufVar + " = x} -> {" + reqObject \
                    + " = 1} -> " + cspRoutine[:-2] \
                    + "_ends -> Skip } else { " + channelList[1] \
                    + "?[x[" + lindx + "] <= " + str(originalSize) + "]x ->{" \
                    + bufVar + " = x} -> {" + reqObject + " = 1} -> " \
                    + cspRoutine[:-2] + "_ends -> Skip [] " \
                    + channelList[0] + "?[x[" + lindx + "] <= " \
                    + str(originalSize) + "]x ->{" + bufVar + " = x} -> {" \
                    + reqObject + " = 1} -> " + cspRoutine[:-2] \
                    + "_ends -> Skip };\n"
            else:
                self.defrData += "if( " + str(originalSize) \
                    + " > threshold ) { "
                for i in range(0, len(channelList), 2):
                    self.defrData += channelList[i] + "?[x[" + lindx \
                    + "] <= " + str(originalSize) + "]x ->{" + bufVar \
                    + " = x} -> {" + reqObject + " = 1} -> " + cspRoutine[:-2] \
                    + "_ends -> Skip "
                    if i + 2 == len(channelList):
                        self.defrData += "}\n\t\t\t\t\t"
                    else:
                        self.defrData += "[]\n\t\t\t\t\t"
                self.defrData += "else { "
                for i in range(0, len(channelList), 2):
                    self.defrData += channelList[i+1] + "?[x[" + lindx \
                    + "] <= " + str(originalSize) + "]x ->{" + bufVar \
                    + " = x} -> {" + reqObject + " = 1} -> " + cspRoutine[:-2] \
                    + "_ends -> Skip [] " + channelList[i] + "?[x[" + lindx \
                    + "] <= " + str(originalSize) + "]x ->{" + bufVar \
                    + " = x} -> {" + reqObject + " = 1} -> " + cspRoutine[:-2] \
                    + "_ends -> Skip "
                    if i + 2 == len(channelList):
                        self.defrData += "};\n"
                    else:
                        self.defrData += "[]\n\t\t\t\t\t"

        # Completion routines for non-blocking routine
        elif routineName == "MPI_Wait":
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            self.defrData += cspRoutine + " = " + cspRoutine[:-2] \
                + "_starts -> "
            reqObject = str(routineArgs[-2])
            self.defrData += "([" + reqObject + " == 1]" + cspRoutine[:-2] \
                + "_ends -> Skip);\n"
        elif routineName == "MPI_Waitany":
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            self.defrData += cspRoutine + " = " + cspRoutine[:-2] \
                + "_starts -> "
            # generating all the request objects in below for loop
            reqObjectInt = int("0" + routineArgs[1], 16)
            count = int(routineArgs[2])
            for i in range(count):
                reqObject = str(hex(reqObjectInt + i * 4))
                # have to remove 0 from starting of reqObject
                if i == count - 1:
                    self.defrData += "([" + reqObject[1:] + " == 1]" \
                        + cspRoutine[:-2] + "_ends -> Skip) "
                else:
                    self.defrData += "([" + reqObject[1:] + " == 1]" \
                        + cspRoutine[:-2] + "_ends -> Skip) [] "
                # a little formatting
                if i > 0 and i % 3 == 0:
                    self.defrData += "\n\t\t\t\t\t"
            self.defrData += ";\n"
        elif routineName == "MPI_Waitall":
            #print(routineArgs)
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            self.defrData += cspRoutine + " = " + cspRoutine[:-2] \
                + "_starts -> "
            # generating all the request objects in below for loop
            reqObjectInt = int("0" + routineArgs[1], 16)
            count = int(routineArgs[2])
            for i in range(count):
                reqObject = str(hex(reqObjectInt + i * 4))
                # have to remove 0 from starting of reqObject
                if i == 0:
                    self.defrData += "([" + reqObject[1:] + " == 1 "
                else:
                    self.defrData += " && " + reqObject[1:] + " == 1 "
                if i == count - 1:
                    self.defrData += "] " + cspRoutine[:-2] + "_ends -> Skip)"
            self.defrData += ";\n"
        elif routineName == "MPI_Barrier":
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            commVar = "c" + str(routineArgs[1])
            commEvent = "e" + str(routineArgs[-2])
            if commVar not in self.definedSet:
                self.bufData += "var " + commVar + " = 0;\n"
                self.definedSet.add(commVar)
            self.defrData += cspRoutine + " = " + cspRoutine[:-2] \
                + "_starts -> "
            self.defrData += "{" + commVar + "++} -> ([" + commVar \
                + " %  num_procs == 0]" + commEvent + " -> Skip);\n"
        elif routineName in collList:
            self.defrData += "// Line " + str(routineArgs[-1]) + "\n"
            collVar = routineName[4:] + "_" \
                + "_".join([str(arg) for arg in routineArgs[1:-1]])
            collEvent = "e" + routineName[4:] \
                + "_" + "_".join([str(arg) for arg in routineArgs[1:-1]])
            if collVar not in self.definedSet:
                self.bufData += "var " + collVar + " = 0;\n"
                self.definedSet.add(collVar)
            self.defrData += cspRoutine + " = " + cspRoutine[:-2] \
                + "_starts -> "
            self.defrData += "{" + collVar + "++} -> ([" + collVar \
                + " %  num_procs == 0]" + collEvent + " -> " + cspRoutine[:-2] \
                + "_ends -> Skip);\n"
        # make an entry that this routine is already created
        # definedSet is older, need to be checked
        self.definedSet.add(cspRoutine)
        self.hashTocspRoutine[hashed_sign] = cspRoutine
        return cspRoutine

    def doChannelCreation(self, routineName, candidateArgs, argString, size):
        '''
        Handles channel creation logic for send routines
        Argument size is used for asynchronous channels
        '''
        # if already created just return channelList
        candidateArgsLocal = candidateArgs
        # if channels are not yet created and a recv routine is
        # called then flip the 0th and f1st args and create channels
        channelList = []
        if routineName == "MPI_Irecv" or routineName == "MPI_Recv":
            # any_source with any_tag case
            if candidateArgs[1] == "A" and candidateArgs[2] == "A":
                for i in range(self.num_procs):
                    for channel in list(self.channelSetDict[str(i)]):
                        recvTarget = channel.split('_')[2]
                        if int(recvTarget) != int(candidateArgs[0]):
                            continue
                        elif channel[-1] == "s":
                            channelList.append(
                                self.createChannelWithName(channel, 0))
                        else:
                            channelList.append(
                                self.createChannelWithName(channel, size))
            # any_tag case
            elif candidateArgs[2] == "A":
                for channel in list(self.channelSetDict[str(
                        candidateArgs[1])]):
                    recvTarget = channel.split('_')[2]
                    if int(recvTarget) != int(candidateArgs[0]):
                        continue
                    if channel[-1] == "s":
                        channelList.append(
                            self.createChannelWithName(channel, 0))
                    else:
                        channelList.append(
                            self.createChannelWithName(channel, size))
            # any_source case
            elif candidateArgs[1] == "A":
                for src in range(self.num_procs):
                    candidateArgsLocal[1] = src
                    newArgString = self.getArgString(candidateArgsLocal, True)
                    channelList.append(
                        self.createChannel(newArgString + "_s", 0))
                    channelList.append(self.createChannel(newArgString, size))
            else:
                channelList.append(self.createChannel(argString + "_s", 0))
                channelList.append(self.createChannel(argString, size))
        else:
            # for send routines create channels
            channelList.append(self.createChannel(argString + "_s", 0))
            channelList.append(self.createChannel(argString, size))

    # print(channelList)
        return channelList

    def createChannelWithName(self, channelName, size):
        if channelName in self.definedSet:
            return channelName
        self.channelData += "channel " + channelName + " " + str(size) + ";\n"
        self.definedSet.add(channelName)
        #print(channelName)
        return channelName

    def createChannel(self, argString, size):
        '''
        Helper method
        Creates a new channel
        '''
        channelName = "ch_" + argString
        if channelName in self.definedSet:
            return channelName
        self.channelData += "channel " + channelName + " " + str(size) + ";\n"
        self.definedSet.add(channelName)
        #print(channelName)
        return channelName

    #def isChannel(self, argString):
    #    '''
    #    Helper method
    #    Checks if channels are already created.
    #    '''
    #    if argString in self.routineToChannels.keys():
    #        return True
    #    else:
    #        return False

    #def setcEntry(self, argString, channelList):
    #    '''
    #    Helper method
    #    Adds channel name to the dictionary of list of channels,
    #    used to avoid creating redundant channels
    #    '''
    #    for channel in channelList:
    #        self.routineToChannels[argString].append(channel)

    #def getcEntry(self, argString):
    #    '''
    #    Helper method
    #    Returns list of channels for a routine if already present
    #    '''
    #    channelList = self.routineToChannels[argString]
    #    return channelList

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

    def createRoutine(self, routineName, routineArgs, reqSet=set()):
        '''
        This method creates equivalent csp encoding for the underlying
        MPI routine calls.
        '''
        #print(list(self.channelSetDict['0']))
        collList = [
            "MPI_Bcast", "MPI_Reduce", "MPI_Gather", "MPI_Scatter",
            "MPI_Allgather", "MPI_Allreduce", "MPI_Alltoall", "MPI_Alltoallv"
        ]
        if routineName == "MPI_Comm_size":
            # gives the number of processes used for execution
            self.num_procs = int(routineArgs[0])
            if "num_procs" not in self.definedSet:
                self.bufData += "var num_procs = " + str(self.num_procs) \
                    + ";\n"
                self.definedSet.add("num_procs")
        elif (routineName == "MPI_Send" or routineName == "MPI_Ssend" \
           or routineName == "MPI_Bsend") and routineArgs[1] != "PNULL":
            # routineArgs(me, dst, tag, count, datatype, bufName, buf, line)
            # candidateArgs reperesent the arguments used for creating
            # csp routineName me-dst-tag
            candidateArgs = routineArgs[:3]
            argString = self.getArgString(candidateArgs, False)
            # size of the channel created in case of asynchronous channel.
            chSize = len(list(routineArgs[-2].split('_'))) + 1
            channelList = self.doChannelCreation(routineName, candidateArgs,
                                                 argString, chSize)
            channelList.sort(reverse=True)
            cspRoutine = self.defineRoutine(routineName, routineArgs,
                                            argString, channelList)
            self.routineList[routineArgs[0]].append(cspRoutine)
        elif routineName == "MPI_Isend" or routineName == "MPI_Issend" \
             or routineName == "MPI_Ibsend":
            if routineArgs[1] == "PNULL":
                # This is the case when MPI_PROC_NULL is used as sender,
                # the method is ignored but the request variable is created
                # since will be required by the wait call.
                reqObject = str(routineArgs[3])
                if reqObject not in self.definedSet:
                    self.bufData += "var " + reqObject + " = 1;\n"
                    self.definedSet.add(reqObject)
            else:
                candidateArgs = routineArgs[:3]
                argString = self.getArgString(candidateArgs, False)
                # size of the channel created in case of asynchronous channel.
                chSize = len(list(routineArgs[-2].split('_'))) + 1
                channelList = self.doChannelCreation(routineName,
                                                     candidateArgs, argString,
                                                     chSize)
                channelList.sort(reverse=True)
                cspRoutine = self.defineRoutine(routineName, routineArgs,
                                                argString, channelList)
                self.routineList[routineArgs[0]].append(cspRoutine)
                self.routineList[routineArgs[0]].append(routineArgs[3])
        elif routineName == "MPI_Recv" and routineArgs[1] != "PNULL":
            # routineArgs(me, src, tag, count, type, logged, symVar, line)
            candidateArgs = routineArgs[:3]
            argString = self.getArgString(candidateArgs, False)
            argStringF = self.getArgString(candidateArgs, True)
            chSize = int(routineArgs[-3]) + 1
            channelList = self.doChannelCreation(routineName, candidateArgs,
                                                 argStringF, chSize)
            channelList.sort(reverse=True)
            cspRoutine = self.defineRoutine(routineName, routineArgs,
                                            argString, channelList)
            self.routineList[routineArgs[0]].append(cspRoutine)
        elif routineName == "MPI_Irecv":
            # irecv(me, srt, tag, request, count, type, loggedElems,
            # bufferName, line)
            # candidateArgs contains me-src-tag
            if routineArgs[1] == "PNULL":
                # This is the case when MPI_PROC_NULL is used as sender,
                # the method is ignored but the request variable is created
                # since will be required by the wait call.
                reqObject = str(routineArgs[3])
                if reqObject not in self.definedSet:
                    self.bufData += "var " + reqObject + " = 1;\n"
                    self.definedSet.add(reqObject)
            else:
                candidateArgs = routineArgs[:3]
                argString = self.getArgString(candidateArgs, False)
                argStringF = self.getArgString(candidateArgs, True)
                # channel recvs the logged elems and size of sender buffer
                chSize = int(routineArgs[-3]) + 1
                channelList = self.doChannelCreation(routineName,
                                                     candidateArgs, argStringF,
                                                     chSize)
                channelList.sort(reverse=True)
                cspRoutine = self.defineRoutine(routineName, routineArgs,
                                                argString, channelList)
                self.routineList[routineArgs[0]].append(cspRoutine)
                self.routineList[routineArgs[0]].append(routineArgs[3])
        elif routineName == "MPI_Wait":
            # waits for an event triggering the routine completion
            if routineArgs[-2] in self.definedSet:
                candidateArgs = routineArgs[:-1]
                argString = self.getArgString(candidateArgs, False)
                cspRoutine = self.defineRoutine(routineName, routineArgs,
                                                argString)
                self.routineList[routineArgs[0]].append(cspRoutine)
        elif routineName == "MPI_Waitany" or routineName == "MPI_Waitall":
            if routineArgs[-2] != "0":
                candidateArgs = routineArgs[:-1]
                argString = self.getArgString(candidateArgs, False)
                cspRoutine = self.defineRoutine(routineName, routineArgs,
                                                argString)
                self.routineList[routineArgs[0]].append(cspRoutine)
        elif routineName == "MPI_Barrier":
            # impleme barrier collective routine
            candidateArgs = routineArgs[:-1]
            argString = self.getArgString(candidateArgs, False)
            cspRoutine = self.defineRoutine(routineName, routineArgs,
                                            argString)
            self.routineList[routineArgs[0]].append(cspRoutine)
        elif routineName in collList:
            # implements other collective routines
            candidateArgs = routineArgs[:-1]
            argString = self.getArgString(candidateArgs, False)
            cspRoutine = self.defineRoutine(routineName, routineArgs,
                                            argString)
            self.routineList[routineArgs[0]].append(cspRoutine)
        elif routineName == "branch":
            bCondList = list(routineArgs[1].split('_'))
            bVarList = []
            # Find the variables used in branch
            for i in range(len(bCondList)):
                if bCondList[i][0] == 'x':
                    bVarList.append(bCondList[i])
                if bCondList[i] == "=":
                    bCondList[i] = "=="
                elif bCondList[i] == "/=":
                    bCondList[i] = "!="
            flag = True
            bVarDict = {}
            for i in range(len(bVarList)):
                vName = "x_" + routineArgs[0] + "_" + bVarList[i][1:]
                if vName not in self.branchVarsTorbufs.keys():
                    flag = False
                    break
                else:
                    bVarDict[bVarList[i]] = self.branchVarsTorbufs[vName]
            if flag:
                for i in range(len(bCondList)):
                    if bCondList[i][0] == 'x':
                        bCondList[i] = bVarDict[bCondList[i]]
                branch = " ".join(bCondList)
                branchDef = "(if(" + branch \
                    + "){Skip} else{{exit_ = true} -> Skip})"
                self.routineList[routineArgs[0]].append(branchDef)
                #print("in here")


class TraceListener(MpiTraceListener, CspCreator):
    '''
    Custom Listener implementation
    '''
    def __init__(self, channelSetDict):
        MpiTraceListener.__init__(self)
        CspCreator.__init__(self, channelSetDict)

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
        # if routineName == "MPI_Scatter":
        # print(args)
        List1 = [
            "MPI_Send", "MPI_Bsend", "MPI_Rsend", "MPI_Ssend", "MPI_Isend",
            "MPI_Ibsend", "MPI_Irsend", "MPI_Issend", "MPI_Recv", "MPI_Irecv"
        ]
        if routineName in List1 and args[1] == "-1":
            args[1] = "PNULL"
        if routineName == "MPI_Waitany":
            count = int(args[-2])
            request = int(args[-3], 16)
            for i in range(count):
                reqSet.add(str(hex(request + i * 4))[1:])
        args = ["C" if x[0:2] == "-3" and '_' not in x else x for x in args]
        args = ["A" if x[0] == "-" and '_' not in x else x for x in args]
        # if routineName == "MPI_Wait":
        #    print(args)
        # Removing 0 from starting of a request objects so as it could directly
        # be used as a name of an event in the csp encoding
        routineArgs = [x[1:] if x[0:2] == "0x" else x for x in args]
        #print(self.channelSetDict)
        self.createRoutine(routineName, routineArgs, reqSet)


class EarlyListener(MpiTraceListener):
    def __init__(self):
        self.channelSetDict = defaultdict(lambda: set())
        self.num_procs = 1
        self.terminated_procs = set()
        # dict from request to nb routine
        # self.nbDict = {}

    def printNonTerminated(self, terminated_procs):
        nonTerminated = list()
        for i in range(self.num_procs):
            if i not in terminated_procs:
                nonTerminated.append(i)
        if len(nonTerminated) > 0:
            strl = ', '.join(str(x) for x in nonTerminated)
            print(
                "\n\n============ Abnormal termination detected ============")
            print("Processes " + strl + " terminated abnormally\n\n")

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
        #if routineName == "branch" or routineName == "MPI_Send":
        #    print(args)
        List11 = [
            "MPI_Send", "MPI_Bsend", "MPI_Rsend", "MPI_Ssend", "MPI_Isend",
            "MPI_Ibsend", "MPI_Irsend", "MPI_Issend", "MPI_Recv", "MPI_Irecv"
        ]
        if routineName == "MPI_Waitany":
            count = int(args[-2])
            request = int(args[-3], 16)
            for i in range(count):
                reqSet.add(str(hex(request + i * 4))[1:])
        args = ["C" if x[0:2] == "-3" and '_' not in x else x for x in args]
        args = ["A" if x[0] == "-" and '_' not in x else x for x in args]
        # Removing 0 from starting of a request objects so as it could directly
        # be used as a name of an event in the csp encoding
        routineArgs = [x[1:] if x[0:2] == "0x" else x for x in args]
        List1 = [
            "MPI_Send", "MPI_Bsend", "MPI_Rsend", "MPI_Ssend", "MPI_Isend",
            "MPI_Ibsend", "MPI_Irsend", "MPI_Issend"
        ]
        # nonBlocking = [
        #     "MPI_Isend", "MPI_Issend", "MPI_Ibsend", "MPI_Irsend", "MPI_Irecv"
        # ]
        if routineName in List1 and routineArgs[1] != "PNULL":
            ch1 = "ch_" + '_'.join([x for x in routineArgs[0:3]])
            ch2 = ch1 + "_s"
            self.channelSetDict[routineArgs[0]].add(ch1)
            self.channelSetDict[routineArgs[0]].add(ch2)
    # if routineName in nonBlocking:
    #     self.nbDict[routineArgs[3]] = 1
        if routineName == "MPI_Comm_size":
            self.num_procs = int(routineArgs[0])
        elif routineName == "MPI_Finalize":
            self.terminated_procs.add(int(routineArgs[0]))

    # elif routineName == "MPI_Wait":
    #     if routineArgs[1] in self.nbDict.keys():
    #         self.nbDict[routineArgs[1]] = 0


def main(argv):
    # argv[1] is MpiTrace file
    input_stream = FileStream(argv[1])
    op_file = "../test/output.csp"
    if len(argv) > 2:
        op_file = argv[2]
    lexer = MpiTraceLexer(input_stream)
    stream = CommonTokenStream(lexer)
    parser = MpiTraceParser(stream)
    tree = parser.trace()
    walker = ParseTreeWalker()
    preprocessor = EarlyListener()
    walker.walk(preprocessor, tree)
    preprocessor.printNonTerminated(preprocessor.terminated_procs)
    printer = TraceListener(preprocessor.channelSetDict)
    walker.walk(printer, tree)
    printer.fileWrite(op_file)


if __name__ == '__main__':
    main(sys.argv)
