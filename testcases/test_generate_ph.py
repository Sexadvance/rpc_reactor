#!/usr/bin/python3.10
import re


# def add_inheritance_and_methods(header_file, source_file):
#     print('=' * 100)

#     print('=' * 100)


#add_inheritance_and_methods('order.pb.h', 'order.pb.cc')
#p = re.findall('[abs]a',s)
s = '1ab 2abb 3abbbbbe 4ba 5ca 6ac 7ddd 8dfsab'
p = re.findall(r'[a-z]{0,2}b$',s)
print(p)



#print(re.findall(r'.a','aaa ba ca ac ddd'))
