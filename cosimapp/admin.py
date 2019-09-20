from django.contrib import admin
from . models import login,notebookdb,recordmesgdb
import builtins
from django.http import HttpResponse,HttpResponseRedirect
from django.shortcuts import render,render_to_response
from django.template.context import RequestContext
from cosimapp.globalvar.GlobalVar import GlobalVarBuf  
#包装csrd 请求，避免django分为其实跨站攻击脚本
#from django.views.decorators.csrf import csrf_exempt
#from django.template.context_processors import csrf
# Register your models here.


class loginitem():
  #  list_display = ('dram','hif')  #页面显示
    def db_save(request):
       # id   = request.POST['id']
        has_regiter = 0
        user = request.POST['username']
        password = request.POST['password']
        tmp = login()

        all_user = login.objects.all() 
       # print(all_user)
        i = 0
        while i < len(all_user):
         #   print(all_user[i])   #debug add 
            
            if user in all_user[i].user:
                has_regiter = 1
               #print(all_user[i].user) #debug add
               #print('has regiter!!!')
            i +=1

        if has_regiter == 0:
          # print('no regiter--')
            tmp.user = user
            tmp.password = password
            tmp.save()
           # return HttpResponse("注册成功")
            return HttpResponseRedirect("/login")
        else :
            return HttpResponse("该账号存在")           
#return HttpResponseRedirect("/q")

    def db_query(request):
        user_true = 0
        pw_true = 0
        user = request.POST['user']
        password = request.POST['password']
        tmp = login()

        all_user = login.objects.all() 
        i = 0
        while i < len(all_user):
            
            if user == all_user[i].user:
                user_true = 1
                if password == all_user[i].password:
                    pw_true = 1
                    break
                else:
                    print('password error!!!')
                    break
            else:
                print('user name error !!!')
               #print('has regiter!!!')
            i +=1

        if (user_true == 1 and pw_true == 1):
#           return HttpResponse("登陆成功")
#           return HttpResponseRedirect("/workpage")
            context = {}
            context['username'] = '用户:'+user
           # print(request.user.username)
           # username = user
           # print(username)
            #global_var(request,user)
            #GlobalVarBuf.__init__()
            GlobalVarBuf.set_value('username',user)
            name = GlobalVarBuf.get_value('username')
            #print('1')
            #print(name)
            #print('2')
            return render(request,'workpage.html',context)
        else :
            return HttpResponse("登陆失败")           
#return HttpResponseRedirect("/q")


admin.site.register(login)
admin.site.register(notebookdb)
admin.site.register(recordmesgdb)
