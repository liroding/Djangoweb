"""myproject URL Configuration

The `urlpatterns` list routes URLs to views. For more information please see:
    https://docs.djangoproject.com/en/2.0/topics/http/urls/
Examples:
Function views
    1. Add an import:  from my_app import views
    2. Add a URL to urlpatterns:  path('', views.home, name='home')
Class-based views
    1. Add an import:  from other_app.views import Home
    2. Add a URL to urlpatterns:  path('', Home.as_view(), name='home')
Including another URLconf
    1. Import the include() function: from django.urls import include, path
    2. Add a URL to urlpatterns:  path('blog/', include('blog.urls'))
"""
from django.contrib import admin
from django.urls import path,include

from django.shortcuts import render
from django.shortcuts import HttpResponse
#sys.path.append('/home/pi/work/djangoproject/myproject/cosimstruct/')
from cosimapp.cosimstruct import cosimstruct
from cosimapp.admin import loginitem
from cosimapp.workhome import workhomeitem
from cosimapp.notebook import notebookitem
from django.views.generic import TemplateView
#登录页面
def login(request):
    #指定要访问的页面，render的功能：讲请求的页面结果提交给客户端
    return render(request,'login.html')
#注册页面
def regiter(request):
    return render(request,'regiter.html')

#首页面
def index(request):
    return render(request,'index.html')
#USER页面
def workpage(request):
    return render(request,'workpage.html')
urlpatterns = [
    path('admin/', admin.site.urls),#系统默认创建的
    
    path('index/', index),#系统默认创建的
    path('login/',login),#用于打开登录页面
    path('regiter/',regiter),#用于打开注册页面
    path('regiter/save',loginitem.db_save),#输入用户名密码后交给后台save函数处理
    path('login/query',loginitem.db_query),#输入用户名密码后交给后台query函数处理
    path('workpage/',workpage),#输入用户名密码后交给后台query函数处理
   # path('workpage/runqemu',workhomeitem.runqemu),#输入用户名密码后交给后台query函数处理

    path('workpage/',include('cosimapp.urls')),#
    path('notebook/',include('cosimapp.urls')),#
    path('hello/',cosimstruct.helloword),#
    path('test/',TemplateView.as_view(template_name='test.txt'))#
]
