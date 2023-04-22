#!/usr/bin/python
#coding:utf-8

import codecs
import os
import shutil
import re
import chardet

def convert_encoding(filename):
	content = codecs.open(filename, 'r').read()
	content = content.replace('gb18030', 'utf-8')
	content = content.decode('gb18030')
	codecs.open(filename, 'w', encoding='utf-8').write(content)

def main():
	for root, dirs, files in os.walk(os.getcwd()):
		for f in files:
			if f.lower().endswith('.xml'):
				filename = os.path.join(root, f)
				try:
					convert_encoding(filename)
				except Exception, e:
					print(e)

main()

