from django.contrib import admin
import os
from . models import login,recordmesgdb,recordquestiondb
import builtins
from django.http import HttpResponse,HttpResponseRedirect
from django.shortcuts import render,render_to_response
from django.template.context import RequestContext
from pygdbmi.gdbcontroller import GdbController
import re
#包装csrd 请求，避免django分为其实跨站攻击脚本
#from django.views.decorators.csrf import csrf_exempt
#from django.template.context_processors import csrf
# Register your models here.


HomePath = os.path.expanduser('~')

class workhomeitem():
  #  list_display = ('dram','hif')  #页面显示
    def runqemu(request):
        print("Start Run Qemu Platform")
        os.system('/home/liroding/workspace/project/startqemu4.0.0.sh bios &')
        return HttpResponse("Run Qemu Starting !!!!")

    def buildcode(request):
        print("Start Build Bios Uefi Code")
        os.system('/home/liroding/workspace/app/zebubuildapp.sh ')
        return HttpResponse("Build Code  Finish !!!!")
    
    def gititemselect(request):
        print("Start git item select")
        return render(request,'gititemselect.html')
    
    def inputmessage(request):
        print("Start inputmessage item")
        return render(request,'inputmessage.html')
    
    def addmesgquery(request):
        index1 = request.POST['index1']
        index2 = request.POST['index2']
        index3 = request.POST['index3']
        index4 = request.POST['index4']
        index5 = request.POST['index5']
        tmp = recordmesgdb()
        print('!!!'+index1)
        if index1:
            print('nihao')
            tmp.index1 = index1
            tmp.index2 = index2
            tmp.index3 = index3
            tmp.index4 = index4
            tmp.index5 = index5
            tmp.save()
        print(index1+index2+index3)        
        all_recordmesg = recordmesgdb.objects.all()
        i = 0
        _dict = {} 
        while i<len(all_recordmesg):
            _dict[i] = {'index1':all_recordmesg[i].index1,'index2':all_recordmesg[i].index2,
                        'index3':all_recordmesg[i].index3,'index4':all_recordmesg[i].index4,
                        'index5':all_recordmesg[i].index5}
            print(_dict[i])
            i=i+1
        return render(request,'showmessage.html',{'mesg_dict':_dict})
    def showmessage(request):
        all_recordmesg = recordmesgdb.objects.all()
        i = 0
        _dict = {} 
        while i<len(all_recordmesg):
            _dict[i] = {'index1':all_recordmesg[i].index1,'index2':all_recordmesg[i].index2,
                        'index3':all_recordmesg[i].index3,'index4':all_recordmesg[i].index4,
                        'index5':all_recordmesg[i].index5}
            print(_dict[i])
            i=i+1
        return render(request,'showmessage.html',{'mesg_dict':_dict})
    def upload(request):
        
        return render(request,'upload.html')

    def showjsondata(request):
        return render(request,'showjson.html')
    def showloginfo(request):
        fd = open('/home/liroding/workspace/app/app.log','r')
        context = fd.read()
        fd.close()
        return render(request,'showloginfo.html',{'context':context})
    def showloginfo_build(request):
        fd = open('/home/liroding/workspace/app/edk2makelog.log','r')
        context = fd.read()
        fd.close()
        return render(request,'showloginfo.html',{'context':context})
    def githandle(request):
        prjname = request.POST['sel_value']
        commit  = request.POST['commit']
        gitpushcmd = prjname + "gitclonepush.git" +" %s"+" %s"
      #  os.system(gitpushcmd % ("push","\""+commit+"\"") )
        os.system(gitpushcmd % ("push",commit))
        return HttpResponse("Git Push Finish")
    def browsedocx(request):
        return render(request,'test.html')
    
    ############## Record Question###################
    def EditRecordQ(request):
        print("Start EditRecord Question item")    
        return render(request,'inputrecordquestion.html')
    def recordquestionquery(request):
        Title = request.POST['title']
        Content = request.POST['content']
        tmp = recordquestiondb()
        if Title:
            tmp.qtitle = Title
            tmp.qcontent = Content
            tmp.save()
        print(Title + Content)        
        all_recordmesg = recordquestiondb.objects.all()
        i = 0
        _dict = {} 
        while i<len(all_recordmesg):
            _dict[i] = {'title':all_recordmesg[i].qtitle,'content':all_recordmesg[i].qcontent}
            print(_dict[i])
            i=i+1
        return render(request,'recordquestion.html',{'mesg_dict':_dict})
    def ShowRecordQ(request):
        all_recordmesg = recordquestiondb.objects.all()
        i = 0
        _dict = {} 
        while i<len(all_recordmesg):
            _dict[i] = {'title':all_recordmesg[i].qtitle,'content':all_recordmesg[i].qcontent}
            print(_dict[i])
            i=i+1
        return render(request,'recordquestion.html',{'mesg_dict':_dict})
    '''
    def AddRecordQ(request):
        index1 = request.POST['index1']
        index2 = request.POST['index2']
        tmp = recordmesgdb()
        print('!!!'+index1)
        if index1:
            tmp.index1 = index1
            tmp.index2 = index2
            tmp.save()
        print(index1+index2)        
        all_recordmesg = recordmesgdb.objects.all()
        i = 0
        _dict = {} 
        while i<len(all_recordmesg):
            _dict[i] = {'index1':all_recordmesg[i].index1,'index2':all_recordmesg[i].index2
                       }
            print(_dict[i])
            i=i+1
        return render(request,'showmessage.html',{'mesg_dict':_dict})
    '''
    ########## [GdbDebugShell]   ############
    def getshellappinfo(request):
        debuglogname = "/home/liroding/workspace/project/debug.log"
        appname = "Zebu_CNX_HIF.efi"
        print(">> Start Copy Appname.debug file")
        os.system('/home/liroding/workspace/app/gdbdebugshellapp.sh ')
        print(">> Start Reading app.log info about .data .txt segment")
       
        #Input GDB CMD
        gdbmi = GdbController()
        response = gdbmi.write('file /home/liroding/workspace/app/Zebu_CNX_HIF.efi')
        response = gdbmi.write('info files')
    #    print(response)
        for _list_ in response:
            #print(_list_)
            _dict = _list_
            #print(_dict['payload'])
            if _dict['payload']:
                if '.text' in _dict['payload']:
                    #example:\t0x000000240 - 0x00000000028120 is .text\n
                    textdata = re.findall(r"t(.+?) -",_dict['payload'])
                    #print (re.findall(r"t(.+?) -",_dict['payload']))
                    print('.text='+textdata[0])
                if '.data' in _dict['payload']:
                    rdata = re.findall(r"t(.+?) -",_dict['payload'])
                    #print (re.findall(r"t(.+?) -",_dict['payload']))
                    print('.data='+rdata[0])
        #return HttpResponse("Get The Info of ShellApp!!!!")
        
        print(">> Get Zebu_CNX_HIF.efi Loading driver Beginaddr")
        fd = open(debuglogname,"r")
        keybaseaddr=''
        for line in fd.readlines():
                if appname in line:
                    #example Loading driver at 0x0007e136000 EntryPoint ...... 
                    data = re.findall(r"t (.+?) E",line)
                    if len(data):
                        print(".base="+data[0])
                        keybaseaddr = data[0]
        
        textsegaddr = int(keybaseaddr,16) + int(textdata[0],16)
        datasegaddr = int(keybaseaddr,16) + int(rdata[0],16)
        print(".base+.text=0x%x" % textsegaddr) 
        print(".base+.data=0x%x" % datasegaddr)

        strtextaddr=hex(textsegaddr)
        strdataaddr=hex(datasegaddr)
        _dict={'.text':textdata[0],'.base':keybaseaddr,'.data':rdata[0],'.textsegaddr':strtextaddr,'.datasegaddr':strdataaddr}
        
        '''
        strtextaddr=hex(textsegaddr)
        strdataaddr=hex(datasegaddr)

        response = gdbmi.write('add-sysmbol-file /home/liroding/workspace/app/Zebu_CNX_HIF.debeg '+strtextaddr+' -s .data '+ strdataaddr)
        print(response)
        response = gdbmi.write('break ShellCEntryLib')
        print(response)
        response = gdbmi.write('target remote localhost:1234')
        print(response)
        response = gdbmi.write('c')
        #return HttpResponse("Get The Info of ShellApp!!!!")
        '''
        return render(request,'gdbdebugshow.html',{'mesg_dict':_dict})
