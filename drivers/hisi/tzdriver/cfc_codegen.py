from cfc_graph import Event, DFAState, Graph
import cfc_graphgen, cfc_rule_parser
import sys

def genCode(graph):
	codes = {}
	codes['$1'] = str(graph.nrEvents)
	codes['$2'] = str(graph.nrDFAStates)
	codes['$3'] = ''
	codes['$4'] = ''
	codes['$5'] = ''

	for symbol in graph.events.values():
		codes['$3'] += 'extern char {}[];\n'.format(symbol.value)

	for event in sorted(graph.events.values(), key = lambda x: x.idx):
		codes['$4'] += '\t\t{},\n'.format(str(event))

	for dfaState in graph.DFAStates:
		codes['$5'] += '\t\t{},\n'.format(dfaState.str(graph.nrEvents))

	return codes

if __name__ == '__main__':
	if (len(sys.argv) < 2):
		raise Exception('command should have 2 args: cfc_codegen.py RuleFilePath')

	# parse rule file and get context
	context = cfc_rule_parser.CFCRuleTextContent()
	context.parse(sys.argv[1])

	# generate DFA graph
	graph = cfc_graphgen.genNFAGraph(context)
	graph.genDFAGraph()

	codes = genCode(graph)

	file = open(sys.path[0] + '/template', 'r')
	template = file.read()
	file.close()

	for k, v in codes.items():
		template = template.replace(k, v)

	sys.stdout.write(template)
