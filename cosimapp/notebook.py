from cosimapp.models import notebookdb
from django.http import HttpResponse
from django.shortcuts import render
import time
from cosimapp.globalvar.GlobalVar import GlobalVarBuf

class notebookitem():
    def get_notebook_list(request):
        notebooks = notebookdb.objects.all()
        ret_str = ''
        for notebook in notebooks:
            a_str = str(notebook.article_id) + ' ' + notebook.title + ' ' + \
                notebook.author + ' ' + str(notebook.publish_date) + ' ' + notebook.content
            ret_str =ret_str+ a_str
        return HttpResponse(ret_str)

    def get_notebook_page(request):
        username = GlobalVarBuf.get_value('username')
        if username:
            notebooks = notebookdb.objects.all()
            return render(request,'mainpage.html',
                    {
                        'articles':notebooks
                    }
                    )
        else:
            return HttpResponse("please login in") 
    def get_detail_page(request):
                      
        articles = notebookdb.objects.all()
        article = None
        id = request.GET['id']
        for a in articles:
            if a.article_id == int(id):
                article = a
                break
        return render(request,'detail.html',
                {
                    'article':article
                })
        pass

                     
    def add_notebook_page(request):
                      
        return render(request,'addnotebook.html')
    
    def add_query_page(request):
                      
        title = request.POST['title']
        content = request.POST['content']
        print(title)
        print(content)
        notebook = notebookdb()
        notebook.title = title
        notebook.content = content
        notebook.save()
        
        notebooks = notebookdb.objects.all()
        return render(request,'mainpage.html',
                {
                    'articles':notebooks
                }
                )
