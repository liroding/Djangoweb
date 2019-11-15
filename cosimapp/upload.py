import os
import sys
from django.http import HttpResponse,HttpResponseRedirect
from django.shortcuts import render,render_to_response
def upload(request):
    if request.method=="POST":
        handle_upload_file(request.FILES['file'],str(request.FILES['file']))        
        return HttpResponse('Successful') 
    print('upload') 
    return render_to_response('upload.html')
 
def handle_upload_file(file,filename):
    pwd = os.getcwd()
    path = pwd + '/uploads/'
    print(path)
    if not os.path.exists(path):
        os.makedirs(path)
    with open(path+filename,'wb+')as destination:
        for chunk in file.chunks():
                destination.write(chunk)
def showuploadfiles(request):
    pwd = os.getcwd()
    path = pwd + '/uploads/'
    _dict = {}
    index = 0
    for root,dirs,files in os.walk(path,topdown=False):
        n = len(files)
        for name in files:
                index = index+1
                _dict[index]= name
 #               _dict[index]= os.path.join(root,name)
                print(os.path.join(root,name))
        index =0
        print(_dict)
        #return render(request,'upload.html',{'mesg_dict':_dict})
        return render(request,'showuploadfiles.html',{'mesg_dict':_dict})
