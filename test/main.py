#!/usr/bin/python3
import pexpect


pathToExe = '../main'
expectTimoutSecs = 1


def spawnAndMatch(cmdArgs, matchPatterns):
	matched = False
	try:
		proc = pexpect.spawn(pathToExe, cmdArgs)
		idx = proc.expect(matchPatterns, expectTimoutSecs)
		if idx == 0:
			matched = True
	except Exception as e:
		print('expected ' + str(matchPatterns) + ' got ' +
			str(proc.before))

	proc.close()
	return matched

def getTestStatus(result):
	if True == result:
		return "PASSED"
	return "FAILED"

def testInvalidMask():
	return getTestStatus(spawnAndMatch(["-m", "xyz"], ["invalid mask x"]))


def testInvalidPath():
	return getTestStatus(spawnAndMatch(["xyz"], ["xyz No such file or directory"]))

def testFileEventInAccess():
	#create a temporary file and do changes to it
	return getTestStatus(spawnAndMatch(["-m", "a", ""], [""]))

testsArr = [testInvalidMask, testInvalidPath, testFileEventInAccess]

if __name__ == '__main__':
	testIdx = 0
	for test in testsArr:
		testIdx += 1
		print ('Test ' + str(testIdx) + ' ' + test.__name__  + ' '+ test() )
