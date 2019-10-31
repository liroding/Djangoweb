from django.urls import path
from cosimapp.workhome import workhomeitem
from cosimapp.notebook import notebookitem
from cosimapp.workhome import workhomeitem
urlpatterns =[
      path('detail',notebookitem.get_detail_page),
      path('mainpage',notebookitem.get_notebook_page),
      path('notebookadd',notebookitem.add_notebook_page),
      path('notebookaddquery',notebookitem.add_query_page),
     
      path('runqemu',workhomeitem.runqemu),
      path('buildcode',workhomeitem.buildcode),


      path('gitselect',workhomeitem.gititemselect),
      path('inputmessage',workhomeitem.inputmessage),
      path('addmesgquery',workhomeitem.addmesgquery),
      path('showmessage',workhomeitem.showmessage),
      path('upload',workhomeitem.upload),
      path('showjsondata',workhomeitem.showjsondata),
      path('showloginfo',workhomeitem.showloginfo),
      path('showloginfobuild',workhomeitem.showloginfo_build),
      path('githandle',workhomeitem.githandle),
      path('browsedocx',workhomeitem.browsedocx),
      ########## [GdbDebugShellApp] #############
      path('getshellappinfo',workhomeitem.getshellappinfo),
]
 
