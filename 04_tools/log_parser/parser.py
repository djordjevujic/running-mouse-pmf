from config import *
from datetime import datetime

fpSplited = fileToParse.split('.')

if len(fpSplited) != 2:
    exit('Invalid name of the input file. Example of the correct one: example.csv')

if fpSplited[1] != "csv":
    exit('Wrong extension of the input file')

# open file which has to be parsed
with open(fileToParse) as fp:
    #TODO: Handle if file is empty
    refLine = fp.readline()
    refLineDate = (refLine.split(delimiters["fields"])[0]).split(delimiters["dateAndTime"])[0].split(delimiters["date"])
    refLineTime = (refLine.split(delimiters["fields"])[0]).split(delimiters["dateAndTime"])[1].split(delimiters["time"])
    refDateTime = datetime(int(refLineDate[0]), int(refLineDate[1]), int(refLineDate[2]), int(refLineTime[0]), int(refLineTime[1]), int(refLineTime[2]))
    #refDateTime = datetime.time(int(refLineTime[0]), int(refLineTime[1]), int(refLineTime[2]))

    nextLine = fp.readline()

    # Open output file, and create if it doesn't exist
    fo = open(fileToParse.split(".")[0]+"_output.csv", "w")
    fo.write("DjordjeKONJ")

    fo.close()

    print(refLine)
    print(nextLine)

