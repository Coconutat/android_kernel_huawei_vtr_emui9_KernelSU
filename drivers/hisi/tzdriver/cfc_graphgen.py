from cfc_graph import Event, NFAState, Graph
from cfc_rule_parser import CFCRuleTextContent, CFCMarker
import re

# find first unit!
# e.g. "a+b+c+d+", it returns the first idx after "a+"
# e.g. "(a+(b+c+))d+", it returns the first idx after "(a+(b+c+))"
# e.g. ".*(a+b+c+)d+", it returns the first idx after ".*"
def findFirstUnit(commandPattern):
	idx = 0
	if commandPattern[0] == '.':
		idx = 1
	elif commandPattern[0] == '(':
		count = 1
		for i in range(1, len(commandPattern)):
			if commandPattern[i] == '(':
				count += 1
			elif commandPattern[i] == ')':
				count -= 1
			if count == 0:
				idx = i + 1
				break
		if count != 0:
			raise Exception('incomplete parenthesis: {}'.format(commandPattern))
	else:
		match = re.match(r'[a-zA-Z_]\w*', commandPattern)
		if match:
			idx = match.end()
	# if no match
	if idx == 0:
		raise Exception('unrecorgnized pattern: {}'.format(commandPattern))
	# check if ending with '*?+'
	if idx < len(commandPattern):
		if commandPattern[idx] == '*' or commandPattern[idx] == '?' or commandPattern[idx] == '+':
			idx += 1
	return idx

# for a given unit, construct graph for it
# we first define the start and end states, then recursively construct the middle sub-graph
# if state A can reach state B with empty event, then we add B in A's directMap and merge A's flags into B's
# see Thompson construction algorithm
def parseCommandPatternUnit(context, graph, start, end, commandPattern):

	# '?':
	# start --- UnitPattern --> end
	#     \                     /
	#      \ --empty event-- > /
	if commandPattern[-1] == '?':
		start.directMap.add(end)
		end.flags |= start.flags
		commandPattern = commandPattern[:-1]

	# '+':
	#                               / <------empty event---- \
	#                              /                          \
	# start --- empty event --> lstate --- UnitPattern ---> rstate --- empty event --> end
	#
	# '*':
	#                               / <------empty event---- \
	#                              /                          \
	# start --- empty event --> lstate --- UnitPattern ---> rstate --- empty event --> end
	#     \                                                                           /
	#      \ --------------------------------empty event---------------------------> /
	if commandPattern[-1] == '+' or commandPattern[-1] == '*':
		lstate = graph.addNFAState()
		rstate = graph.addNFAState()

		lstate.flags = start.flags

		start.directMap.add(lstate)
		rstate.directMap.add(lstate)
		rstate.directMap.add(end)

		if commandPattern[-1] == '*':
			start.directMap.add(end)
			end.flags |= start.flags

		start = lstate
		end = rstate
		commandPattern = commandPattern[:-1]

	if commandPattern == '.':
		start.addOutEdge(graph.getEvent(commandPattern), end)
		return

	# recursively construct sub-graph for '(...)'
	if commandPattern[0] == '(':
		parseCommandPattern(context, graph, start, end, commandPattern[1:-1])
		return

	# check if the var is known
	if commandPattern != '.' and not context.markers.has_key(commandPattern):
		raise Exception('unrecorgnized var: {}'.format(commandPattern))

	# only ATTRIBUTE_SEND_DATA_START event can reach FLAG_WAIT_DATA state
	if context.markers[commandPattern].attribute == CFCMarker.ATTRIBUTE_SEND_DATA_START:
		end.flags |= NFAState.FLAG_WAIT_DATA
	elif end.flags & NFAState.FLAG_WAIT_DATA != 0:
		raise Exception('only ATTRIBUTE_SEND_DATA_START event can reach FLAG_WAIT_DATA state {}'.format(commandPattern))

	eventValue = context.markers[commandPattern].symbol
	start.addOutEdge(graph.getEvent(eventValue), end)

# 1. split the commandPattern into single units (only first layer)
#    e.g. a+b+c+ -> [[a+], [b+], [c+]]
#    e.g. a+(b+|(d+ e+))c+ -> [[a+], [b+, (d+ e+)], [c+]], (d+ e+) will be handled in deeper layer
# 2. concatenate (or union) these units
def parseCommandPattern(context, graph, start, end, commandPattern):
	units = []
	parallelPatterns = []
	commandPattern = commandPattern.strip()
	while len(commandPattern) != 0:
		idx = findFirstUnit(commandPattern)
		parallelPatterns.append(commandPattern[:idx])
		commandPattern = commandPattern[idx:].strip()
		if len(commandPattern) == 0 or commandPattern[0] != '|':
			units.append(parallelPatterns)
			parallelPatterns = []
		if len(commandPattern) != 0 and commandPattern[0] == '|':
			commandPattern = commandPattern[1:].strip()

	# empty
	if len(units) == 0:
		start.directMap.add(end)
		return
	# union
	if len(units) == 1:
		for pattern in units[0]:
			parseCommandPatternUnit(context, graph, start, end, pattern)
		return

	# concatenation
	for i in range(0, len(units)):
		nextEnd = end
		if i < len(units) - 1:
			nextEnd = graph.addNFAState()
		# union
		for pattern in units[i]:
			parseCommandPatternUnit(context, graph, start, nextEnd, pattern)
		start = nextEnd


# construct NFA graph with given context
# 1. add all events into graph
# 2. construct sub-graph for each commandPattern
def genNFAGraph(context):
	graph = Graph()
	finalStates = {}

	# add events into graph
	graph.addEvent(Event.FLAG_WILDCARD, '.')
	for var, marker in sorted(context.markers.items(), key = lambda x: x[1].attribute):
		if marker.attribute == CFCMarker.ATTRIBUTE_RESET:
			graph.addEvent(Event.FLAG_RESET, marker.symbol)
		elif marker.attribute == CFCMarker.ATTRIBUTE_SEND_DATA_STOP:
			graph.addEvent(Event.FLAG_WAIT_DATA, marker.symbol)
		elif marker.attribute == CFCMarker.ATTRIBUTE_GLOBAL:
			graph.addEvent(Event.FLAG_GLOBAL, marker.symbol)
		else:
			graph.addEvent(0, marker.symbol)

	start = graph.addNFAState()
	for commandID, commandPattern in context.commandPatterns:
		if not finalStates.has_key(commandID):
			finalStates[commandID] = graph.addNFAState().setSucceed().setStop().setCMD(int(commandID))
		parseCommandPattern(context, graph, start, finalStates[commandID], commandPattern)

	graph.completeNFAGraph()
	return graph
