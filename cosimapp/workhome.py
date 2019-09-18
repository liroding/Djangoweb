from django.contrib import admin
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
        a=1
        b=2
        c=a+b
        return HttpResponse("Run Qemu Starting !!!!")

