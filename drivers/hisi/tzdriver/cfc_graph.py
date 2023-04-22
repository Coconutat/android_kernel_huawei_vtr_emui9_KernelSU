class Event(object):
	# FLAG_GLOBAL events triggers events for all existed state machines
	# FLAG_RESET events force state machine to enter default final state
	# FLAG_WAIT_DATA events make stream id become unique identifier
	FLAG_GLOBAL = 0x1
	FLAG_RESET = 0x2
	FLAG_WAIT_DATA = 0x4
	FLAG_WILDCARD = 0x8000

	def __init__(self, flags, value, idx):
		self.idx = idx
		self.flags = flags
		self.value = value

	def __eq__(self, other):
		return self.value == other.value

	def __hash__(self):
		return hash(self.value)

	def __str__(self):
		return '{{{}, (u64){}}}'.format(self.flags, self.value)

class NFAState(object):
	FLAG_STOP = 1
	FLAG_SUCC = 2
	FLAG_WAIT_DATA = 4
	FLAG_CMD_OFFSET = 8
	FLAG_CMD_MASK = 0xff

	def __init__(self, idx):
		self.idx = idx
		self.flags = 0
		self.transMap = {} # event -> NFAState map
		self.directMap = set() # empty -> NFAState map
		self.fullDirectMap = False

	def __eq__(self, other):
		return self.idx == other.idx

	def __hash__(self):
		return self.idx

	def setStop(self):
		self.flags |= NFAState.FLAG_STOP
		return self

	def setSucceed(self):
		self.flags |= NFAState.FLAG_SUCC
		return self

	def setCMD(self, cmd):
		self.flags |= (cmd & NFAState.FLAG_CMD_MASK) << (NFAState.FLAG_CMD_OFFSET)
		return self

	# add an out state for a given event
	def addOutEdge(self, event, state):
		if not self.transMap.has_key(event):
			self.transMap[event] = set()
		self.transMap[event].add(state)

	# add a set of out states for a given event
	def addOutEdges(self, event, states):
		if not self.transMap.has_key(event):
			self.transMap[event] = set()
		self.transMap[event] |= states

	# a state that can be reached with empty input called directMap (forgiving me for my poor naming skill)
	# this function generates all directMap state, e.g. S1->S2->S3->S4 then [S3, S4] should also be in S1's directMap
	# for all states in (S1's) top-directMap, we merge their sub-directMaps into top-directMap iteratively
	# if a state has already generated full directMap, we can skip the iterations for its sub-directMap
	def genFullDirectMap(self):
		processSet = set(self.directMap)
		while len(processSet) > 0:
			nfaState = processSet.pop()
			if nfaState.fullDirectMap:
				self.directMap |= nfaState.directMap
				continue
			# '-' is Set Diff, e.g. ([1, 2, 3]) - ([1]) = ([2, 3])
			diffDirectMap = nfaState.directMap - self.directMap
			processSet |= diffDirectMap
			self.directMap |= diffDirectMap
		self.fullDirectMap = True

class DFAState(object):
	def __init__(self):
		self.idx = Graph.DFASTATE_INVALID_IDX
		self.flags = 0
		self.NFAStates = set()
		self.transMap = {} # event -> dfaState

	def __eq__(self, other):
		return self.idx == other.idx or self.NFAStates == other.NFAStates

	# add a NFAState into NFAStateSet, also merge the flags
	# TODO: need a conflict detection method
	def addNFAStateSingle(self, nfaState):
		if nfaState not in self.NFAStates:
			self.NFAStates.add(nfaState)
			self.flags = self.flags | nfaState.flags
			return self

	# add a NFAState and its directMapped NFAStates into NFAStateSet
	# this function should be invoked after NFAState has generated full directMap
	def addNFAState(self, nfaState):
		if not nfaState.fullDirectMap:
			raise Exception('addNFAState invoked for a state before generating full directMap')
		if nfaState not in self.NFAStates:
			self.addNFAStateSingle(nfaState)
			for subNFAState in nfaState.directMap:
					self.addNFAStateSingle(subNFAState)
		return self

	# generate the next DFAState containing all reachable NFAStates with the given event
	def genNextDFAState(self, event):
		nextDFAState = DFAState()
		for nfaState in self.NFAStates:
			if not nfaState.transMap.has_key(event):
				continue
			for nextNFAState in nfaState.transMap[event]:
				nextDFAState.addNFAState(nextNFAState)

		return nextDFAState

	def str(self, eventLen):
		prefix = ''
		nextStatesStr = ''
		nextStatesIdx = [0] * eventLen

		for event, dfaState in self.transMap.items():
			nextStatesIdx[event.idx] = dfaState.idx

		for dfaStatIdx in nextStatesIdx:
			nextStatesStr += prefix + str(dfaStatIdx)
			prefix = ', '

		return '{{{}, {{{}}}}}'.format(self.flags, nextStatesStr)


class Graph(object):
	# guaranteed by algorithm
	STATE_STOP_IDX = 0
	STATE_START_IDX = 1

	# pseudo index
	EVENT_WILDCARD_IDX = -1
	EVENT_INVAID_IDX = -2
	DFASTATE_INVALID_IDX = -1

	def __init__(self):
		self.nrEvents = 0
		self.nrNFAStates = 0
		self.nrDFAStates = 0
		self.events = {}
		self.NFAStates = {}
		self.DFAStates = []
		# NFA States[0] is default failure state
		self.addNFAState().setStop()

	# add an event in dict and give it a sequential index
	def addEvent(self, flags, value):
		if not self.events.has_key(value):
			if flags & Event.FLAG_WILDCARD:
				self.events[value] = Event(flags, value, Graph.EVENT_WILDCARD_IDX)
			else:
				self.events[value] = Event(flags, value, self.nrEvents)
				self.nrEvents += 1

	# get the event identified by value
	def getEvent(self, value):
		return self.events[value]

	# add an NFA State and give it an idx, NFA State is identified by idx
	def addNFAState(self):
		nfaState = NFAState(self.nrNFAStates)
		self.NFAStates[self.nrNFAStates] = nfaState
		self.nrNFAStates += 1
		return nfaState

	def getNFAState(self, idx):
		return self.NFAStates[idx]

	# add an DFA State and give it an idx, DFA State is identified by idx
	def addDFAState(self, dfaState):
		if dfaState in self.DFAStates:
			return (self.DFAStates[self.DFAStates.index(dfaState)], False)
		dfaState.idx = self.nrDFAStates
		self.DFAStates.append(dfaState)
		self.nrDFAStates += 1
		return (dfaState, True)

	def getDFAState(self, idx):
		return self.DFAStates[idx]

	# works that can be done after NFA graph is completed:
	def completeNFAGraph(self):
		# 1. For GLOBAL events, non-STOP states (without out edge for these events) accepts them as loopback
		for event in filter(lambda x: x.flags & Event.FLAG_GLOBAL != 0, self.events.values()):
			for nfaState in filter(lambda x: x.flags & NFAState.FLAG_STOP == 0 and not x.transMap.has_key(event), self.NFAStates.values()):
				nfaState.addOutEdge(event, nfaState)

		# 2. For RESET events, non-STOP states accept them and go to STATE_STOP_IDX state
		for event in filter(lambda x: x.flags & Event.FLAG_RESET != 0, self.events.values()):
			for nfaState in filter(lambda x: x.flags & NFAState.FLAG_STOP == 0, self.NFAStates.values()):
				nfaState.addOutEdge(event, self.getNFAState(Graph.STATE_STOP_IDX))

		# 3. For WILDCARD events, replace it with all non-RESET events (there should be only 1 WILDCARD event)
		for event in filter(lambda x: x.flags & Event.FLAG_WILDCARD != 0, self.events.values()):
			for nfaState in filter(lambda x: x.transMap.has_key(event), self.NFAStates.values()):
				wildcastNextNFAStates = nfaState.transMap.pop(event)
				for otherEvent in self.events.values():
					if otherEvent.flags & (Event.FLAG_RESET | Event.FLAG_WILDCARD) == 0:
						nfaState.addOutEdges(otherEvent, wildcastNextNFAStates)

		# drop the wildcast events because it's useless now
		self.events = dict(filter(lambda x: x[1].flags & Event.FLAG_WILDCARD == 0, self.events.items()))


	# 1. generate full directMap for each state
	# 2. BFS, foreach DFAState, enumerate its reachable DFAState and add it into Graph
	def genDFAGraph(self):
		for nfaState in self.NFAStates.values():
			nfaState.genFullDirectMap()

		self.addDFAState(DFAState().addNFAState(self.getNFAState(Graph.STATE_STOP_IDX)))
		self.addDFAState(DFAState().addNFAState(self.getNFAState(Graph.STATE_START_IDX)))
		processingDFAStates = set([])
		processingDFAStates.add(self.getDFAState(Graph.STATE_START_IDX))

		while len(processingDFAStates) > 0:
			curDFAState = processingDFAStates.pop()
			for event in self.events.values():
				nextDFAState = curDFAState.genNextDFAState(event)
				if len(nextDFAState.NFAStates) == 0:
					nextDFAState = self.getDFAState(Graph.STATE_STOP_IDX)
				else:
					nextDFAState, added = self.addDFAState(nextDFAState)
					if added:
						processingDFAStates.add(nextDFAState)
				curDFAState.transMap[event] = nextDFAState
