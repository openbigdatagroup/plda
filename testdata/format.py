#!/usr/bin/env python
import io
import sys

def parseLine(line):
    l = line.strip().split()
    return int(l[0]), int(l[1]), int(l[2])

def convert(dataName, f):
    with open("docword." + dataName + ".txt", 'r') as doc, open("vocab." + dataName + ".txt", 'r') as voc:
        vocList = []
        for w in voc:
            vocList.append(w.strip())
        print vocList
        docs = doc.readlines()
        docNum = int(docs[0])
        wordNum = int(docs[1])
        assert wordNum == len(vocList)
    
        docIndex, wordIndex, count = parseLine(docs[3])
        newLine = str(vocList[wordIndex-1] + " " + str(count)) + " "
        for i in range(4, len(docs)):
            print i
            d, w, c = parseLine(docs[i])
            #print docs[i] 
            if d == docIndex:
                newLine = newLine + vocList[w-1] + " " + str(c) + " "
            else :
                docIndex = d
                f.write(newLine + "\n")
                newLine = str(vocList[w-1] + " " + str(c)) + " "




if __name__ == '__main__':
    if len(sys.argv) < 2:
        print "no argument"
        exit()

    dataName = sys.argv[1]
    print dataName

    with open(dataName + ".txt", 'w') as of:
        convert(dataName, of)







