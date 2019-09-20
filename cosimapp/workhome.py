from django.contrib import admin
import os
from . models import login,recordmesgdb
import builtins
from django.http import HttpResponse,HttpResponseRedirect
from django.shortcuts import render,render_to_response
from django.template.context import RequestContext
#包装csrd 请求，避免django分为其实跨站攻击脚本
#from django.views.decorators.csrf import csrf_exempt
#from django.template.context_processors import csrf
# Register your models here.


class workhomeitem():
  #  list_display = ('dram','hif')  #页面显示
    def runqemu(request):
        print("Start Run Qemu Platform")
        os.system('/home/liroding/workspace/project/startqemu.sh bios &')
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
