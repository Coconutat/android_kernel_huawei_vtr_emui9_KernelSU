import re

class CFCMarker(object):
	ATTRIBUTE_NONE = 0
	ATTRIBUTE_SEND_DATA_START = 1
	ATTRIBUTE_SEND_DATA_STOP = 2
	ATTRIBUTE_GLOBAL = 3
	ATTRIBUTE_RESET = 4

	def __init__(self, symbol, attribute = ''):
		self.symbol = symbol
		if not attribute:
			self.attribute = CFCMarker.ATTRIBUTE_NONE
		elif attribute == 'RESET':
			self.attribute = CFCMarker.ATTRIBUTE_RESET
		elif attribute == 'SEND_DATA_START':
			self.attribute = CFCMarker.ATTRIBUTE_SEND_DATA_START
		elif attribute == 'SEND_DATA_STOP':
			self.attribute = CFCMarker.ATTRIBUTE_SEND_DATA_STOP
		elif attribute == 'GLOBAL':
			self.attribute = CFCMarker.ATTRIBUTE_GLOBAL
		else:
			raise Exception('unexpect attribute: {}'.format(attribute))

	def __str__(self):
		return 'symbol: {}, attribute: {}'.format(symbol, attribute)

class CFCRuleTextContent(object):
	def __init__(self):
		self.markers = {}
		self.commandPatterns = []
		self.symbols = set([])
		self.lineNum = 0;
		self.patternWord = re.compile(r'[a-zA-Z_]\w*$')
		self.patternNum = re.compile(r'(0(x|b|d|o))?\d+$')

	# parse file in lines
	def parse(self, filePath):
		file = open(filePath, 'r')
		for s in file:
			self.lineNum += 1;
			self.parseLine(s)
		file.close()

	def parseLine(self, s):
		s = ' '.join(s.split())
		# skip empty or comments line
		if not s or s[0] == '#':
			return

		# only two kinds of sentences we expected handle
		# 1. Define Marker - Var := Symbol[, Attribute]
		# 2. Define Pattern - CommandID = $RegexExpression
		if ':=' in s:
			self.parseMarker(s)
		elif '=' in s:
			self.parsePattern(s)
		else:
			raise Exception('Line {}: unrecoginzed line, {}.'.format(self.lineNum, s))

	# Var := Symbol[, Attribute]
	def parseMarker(self, s):
		s = s.replace(',', ' ')
		tokens = filter(bool, s.split())
		if len(tokens) < 3 or len(tokens) > 4:
			raise Exception('Line {}: unexpected tokens numbers {}.'.format(self.lineNum, len(tokens)))
		if tokens[1] != ':=':
			raise Exception('Line {}: unexpected second tokens {}.'.format(self.lineNum, tokens[1]))
		if not self.patternWord.match(tokens[0]):
			raise Exception('Line {}: unexpected tokens {}.'.format(self.lineNum, tokens[0]))
		if not self.patternWord.match(tokens[2]):
			raise Exception('Line {}: unexpected tokens {}.'.format(self.lineNum, tokens[2]))

		varname = tokens[0]
		symbol = tokens[2]
		attribute = ''
		if len(tokens) == 4:
			attribute = tokens[3]
		if varname in self.markers:
			raise Exception('Line {}: duplicated definition for {}.'.format(self.lineNum, varname))
		if symbol in self.symbols:
			raise Exception('Line {}: duplicated symbol {}.'.format(self.lineNum, symbol))

		self.markers[varname] = CFCMarker(symbol, attribute)
		self.symbols.add(symbol)

	# CommandID = $RegexExpression
	def parsePattern(self, s):
		s = ' '.join(s.split())
		tokens = s.split(' ', 2)
		if len(tokens) != 3:
			raise Exception('Line {}: unexpected tokens numbers {}.'.format(self.lineNum, len(tokens)))
		if not self.patternNum.match(tokens[0]):
			raise Exception('Line {}: unexpected tokens {}, should be numeric.'.format(self.lineNum, tokens[0]))
		if tokens[1] != '=':
			raise Exception('Line {}: unexpected tokens {}.'.format(self.lineNum, tokens[2]))

		self.commandPatterns.append((tokens[0], tokens[2]))
