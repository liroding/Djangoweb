from django.contrib import admin
import os
from . models import login
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
