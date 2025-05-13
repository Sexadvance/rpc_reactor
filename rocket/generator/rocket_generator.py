#!/usr/bin/python3.10
import getopt
import traceback
import os
from string import Template
import sys
from datetime import datetime
import re

project_name = ""           # 会根据proto文件的名字截取一部分作为项目名
proto_file = ""             # proto文件绝对路径
out_project_path = "./"     # 输出文件夹
bin_path = ""
conf_path = ""
test_client_path = ""
test_client_tool_path = ""
src_path = ""               # service地址 
generate_path = ""          # 模板地址

def parseInput():
    opts,args = getopt.getopt(sys.argv[1:], "hi:o:", longopts=["help", "input=", "output="])

    for opt,arg in opts:
        if opt == "-h" or opt == "--help":
            printHelp()
            sys.exit(0)
        if opt == "-i" or opt == "--input":
            global proto_file
            proto_file = arg
            last_slash_index = proto_file.rfind('/')
            global project_name
            project_name = proto_file[last_slash_index + 1:]
        elif opt == "-o" or opt == "--output_path" :
            global out_project_path
            out_project_path = arg
        else:
            raise Exception("invaild options:[" + opt + ": " + arg + "]")
    
    if not os.path.exists(proto_file):
        raise Exception("Generator error, not exist protobuf file: " + proto_file)

    if ".proto" not in proto_file:
        raise Exception("Generator error, input file isn't standard protobuf file:[" + proto_file + "]")

def printHelp():
    print('please point the input and output')
    print('please make sure your path is absolute path rather than relative path')


def generate_dir():
    print('=' * 75)
    print('Begin to generate project dir')

    global out_project_path
    proj_path = out_project_path + '/server'

    global bin_path
    bin_path = out_project_path + '/bin'

    global conf_path
    conf_path = out_project_path + '/conf'

    lib_path = out_project_path + '/lib'
    obj_path = out_project_path + '/obj'

    global test_client_path
    test_client_path = out_project_path  + '/test_client'

    global test_client_tool_path
    test_client_tool_path = test_client_path + '/test_tool'

    log_path = proj_path + '/log'

    global src_path
    src_path = proj_path
    src_interface_path = src_path + '/interface'
    src_server_path = src_path + '/' + project_name[:-6]
    src_pb_path = src_path + '/pb'
    src_comm_path = src_path + '/comm'

    dir_list = []
    dir_list.append(out_project_path)
    dir_list.append(bin_path)
    dir_list.append(conf_path)
    dir_list.append(lib_path)
    dir_list.append(obj_path)

    dir_list.append(src_path)
    dir_list.append(src_comm_path)
    dir_list.append(src_interface_path)
    dir_list.append(log_path)
    dir_list.append(src_pb_path)
    dir_list.append(src_server_path)

    dir_list.append(test_client_path)
    dir_list.append(test_client_tool_path)

    for each in dir_list:
        if not os.path.exists(each):
            os.mkdir(each)
        print("succ generate project dir" + each)

    print('End generate project dir')
    print('=' * 75)

def generate_pb():
    print('=' * 75)
    print('Begin to generate protobuf file')
    pb_path = src_path + '/pb'
    cmd = 'cp -r ' + proto_file + ' ' + pb_path
    cmd += ' && cd ' + pb_path + ' && protoc --cpp_out=./ '  + project_name
    print('excute cmd:' + cmd)
    if os.system(cmd) != 0:
        raise Exception("excute cmd failed[" + cmd + "]")
    
    print("succ generate pb")
    
    print('End generateprotobuf file')
    print('=' * 75)

def generate_makefile():
    print('=' * 75)
    print('Begin to generate makefile')
    out_file = out_project_path + '/makefile'
    if os.path.exists(out_file):
        print('makefile exists, skip generate')
        print('End generate makefile')
        print('=' * 75)
        return
    
    global generate_path
    template_file = open(generate_path + '/template/makefile.template', 'r')
    tmpl = Template(template_file.read())

    content = tmpl.safe_substitute(
        PROJECT_NAME = project_name[:-6],
        CREATE_TIME = datetime.now().strftime('%Y-%m-%d %H:%M;%S'))

    file = open(out_file, 'w')
    file.write(content)
    file.close()
    print('succ write to ' +  out_file)
    print('End generate makefile')
    print('=' * 75)

def generate_main():
    print('='  * 75)
    print('Begin to generate main')

    out_file = src_path + '/' + project_name[:-6] + '/' + project_name[:-6] + '.cc'
    if os.path.exists(out_file):
        print('main exists, skip generate')
        print('End generate main')
        print('=' * 75)
        return
    
    vir_file = open(src_path + '/pb/' + project_name[:-6] + '.pb.h')
    content =  vir_file.read()
    vir_file.close()
    class_name = re.search(r'class\s(.*?)\s:\spublic\s::PROTOBUF_NAMESPACE_ID::Service',content)
    class_name = class_name.group(1)

    template_file  = open(generate_path + '/template/main.cc.template')
    tmpl = Template(template_file.read())
    content = tmpl.safe_substitute(PROJECT_NAME = project_name[:-6],SERVICE_NAME = class_name,FILE_NAME = project_name[:-6] + '.cc')

    file = open(out_file,'w')
    file.write(content)
    file.close()

    print('End generate main')
    print('=' * 75)

def generate_Impl_h():
    print('='  * 75)
    print('Begin to generate impl_h')

    global src_path
    out_file = src_path + '/interface' + '/impl.h'
    if os.path.exists(out_file):
        print('Impl exists, skip generate')
        print('End generate impl_h')
        print('=' * 75)
        return

    #################################################################################################
    # 读取pb文件中需要实现的虚函数
    vir_file = open(src_path + '/pb/' + project_name[:-6] + '.pb.h')
    content =  vir_file.read()
    vir_file.close()
    vir_content = re.findall(r'virtual\svoid\s.+?;',content,flags=re.DOTALL)

    # for i in vir_content:
    #     print(i)
    vir_func = []
    for i in vir_content:
        i = i[7:]
        vir_func.append(re.sub(r'::PROTOBUF_NAMESPACE_ID::RpcController\*','google::protobuf::RpcController*',i))

    # class Order : public ::PROTOBUF_NAMESPACE_ID::Service
    class_name = re.search(r'class\s(.*?)\s:\spublic\s::PROTOBUF_NAMESPACE_ID::Service',content)
    # print('a'*100)
    # print(class_name.group(1))

    # for i in vir_func:
    #     print(i)
    ##################################################################################################

    ##################################################################################################
    # 读取，替换，追加模板
    template_file = open(generate_path + '/template/impl.h.template','r')
    tmpl = Template(template_file.read())
    content = tmpl.safe_substitute(
        PROJECT_NAME = '../pb/' + project_name[:-6],
        SERVICE_NAME = class_name.group(1))
    
    content = content[:-2] + '\n'.join(vir_func) + '\n' + content[-2:]
    # print(content)

    file = open(out_file,'w')
    file.write(content)
    file.close()
    ##################################################################################################

    print('End generate impl')
    print('=' * 75)

def generate_Impl_cc():
    print('='  * 75)
    print('Begin to generate impl_cc')

    global src_path
    out_file = src_path + '/interface' + '/impl.cc'
    if os.path.exists(out_file):
        print('Impl exists, skip generate')
        print('End generate impl_cc')
        print('=' * 75)
        return

    ###################################################################################################
    # 读取pb文件，实现虚函数
    content = '#include "impl.h"'
    vir_file = open(src_path + '/pb/' + project_name[:-6] + '.pb.h')
    vir_content =  vir_file.read()
    vir_file.close()
    vir_content = re.findall(r'virtual\svoid\s.+?;',vir_content,flags=re.DOTALL)
    vir_func = []
    for i in vir_content:
        i = i[8:13] + 'Impl::' + i[13:]
        i = i[:-1] + '{\n' + '/'*20  + 'implement your service' + '/'*20 + '\n\n' + '}'
        vir_func.append(re.sub(r'::PROTOBUF_NAMESPACE_ID::RpcController\*','google::protobuf::RpcController*',i))
    ###################################################################################################

    file = open(out_file,'w')
    content += '\n\n' + '\n\n'.join(vir_func)
    # print(content)
    file.write(content)
    file.close()

    print('End to generate impl_cc')
    print('='  * 75)

def generate_conf():
    print('='*75)
    print('begin to generate conf')

    out_file = out_project_path + '/conf' + '/config.xml'
    if os.path.exists(out_file):
        print('config.xml exists, skip generate')
        print('End generate rocket.xml')
        print('=' * 75)
        return

    template_file = open(generate_path + '/template/config.xml.template','r')
    tmpl = Template(template_file.read())
    content = tmpl.safe_substitute(
        PROJECT_NAME = project_name[:-6])

    file = open(out_file,'w')
    file.write(content)
    file.close()

    print('End to generate conf')
    print('=' * 75)


def generate_project():
    try:
        parseInput()# 获得输入和输出参数
        print('=' * 125)
        print("Begin to generate rocket rpc server")
        print(os.path.abspath(__file__))
        global generate_path
        generate_path = os.path.abspath(__file__)[:-20]

        generate_dir()

        generate_pb()

        generate_makefile()

        generate_main()

        generate_Impl_h()

        generate_Impl_cc()

        generate_conf()

        print('end generate rocket rpc server')
        print('=' * 125)
        
    except Exception as e:
        print("failed to generate rocket rpc server, err:" + e)
        sys.exit(0)
        print('=' * 125)

if __name__ == '__main__':
    generate_project()
