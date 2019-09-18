from django.contrib import admin
import os
class cosimstruct():
    def helloword(request):
        print('nihao')
class sqlitehandle():
    def save(request):
        return HttpResponse('save')
    def query(request):
        return HttpResponse('query') 


