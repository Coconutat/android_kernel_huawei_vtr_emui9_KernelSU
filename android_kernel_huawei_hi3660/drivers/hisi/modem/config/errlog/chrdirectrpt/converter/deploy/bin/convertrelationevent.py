#!/usr/bin/python
#coding:utf-8

import re

class Event:
	event = ''
	subevents = []
	
	def __init__(self, event, subevent):
		self.event = event
		self.subevents = []
		self.subevents.append(subevent)
		
	def __str__(self):
		return self.event + ":" + ",".join(self.subevents)

result = []

def AddSubItem(event, subevent):
	for r in result:
		if r.event == event:
			r.subevents.append(subevent)
			return
	ev = Event(event, subevent)
	result.append(ev)

def Output(path):
	file = open(path, 'w')
	s = ''
	for r in result:
		s += 'typedef struct\n'
		s += '{\n'
		for f in r.subevents:
			s += '	' + f + ' st' + f + ';\n'
		s += '} ' + r.event + ';\n'
		s += r.event + ' g_st' + r.event + ';\n'
	file.write(s)
	file.close()
	
def ConvertRelationEvent(path):
	file = open(path, 'r')
	data = file.read()
	m = '#define[\s]+RELATION_EVENT_DEFINITION([\s\S]*)#undef[\s]+RELATION_EVENT_DEFINITION'
	text = re.findall(m, data)
	print text
	for r in text:
		m = '\{[\s]*([^\s,]*)[\s]*,[\s]*([^\s,]*)[\s]*,[\s]*([^\s,]*)[\s]*\}'
		items = re.findall(m, r)
		for i in items:
			AddSubItem(i[0][:i[0].index('_FAULTID')], i[2][:i[2].index('_ALARMID')])
	file.close()
	
def ConvertAbsoluteEvent(path):
	file = open(path, 'r')
	data = file.read()
	m = '.*int[\s]+g_[^\[]*\[[^\]]*\][\s]*\[[^\]]*\][\s]*=[\s]*\{[\s]*([^;]*)\}[\s]*;'
	text = re.findall(m, data)
	for r in text:
		m = '\{[\s]*([^\s,]*)[\s]*,[\s]*([^\s,]*)[\s]*\}'
		items = re.findall(m, r)
		for i in items:
			AddSubItem(i[0][:i[0].index('_EVENTID')], i[1][:i[1].index('_ALARMID')])
	file.close()

ConvertRelationEvent('../../../inc/ChrRelationEvent_cfg.h')
ConvertAbsoluteEvent('../../../src/ChrAbsoluteEvent_cfg.c')
Output('./ChrRelationEvent.c')