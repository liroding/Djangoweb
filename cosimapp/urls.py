from django.urls import path
from cosimapp.workhome import workhomeitem
from cosimapp.notebook import notebookitem
from . import upload
urlpatterns =[
      path('detail',notebookitem.get_detail_page),
      path('mainpage',notebookitem.get_notebook_page),
      path('notebookadd',notebookitem.add_notebook_page),
      path('notebookaddquery',notebookitem.add_query_page),
      path('modifynotebook',notebookitem.modifynotebook),
      path('modifynotebookquery',notebookitem.modifynotebook_query),
     
      path('runqemu',workhomeitem.runqemu),
      path('buildcode',workhomeitem.buildcode),


      path('gitselect',workhomeitem.gititemselect),
      path('inputmessage',workhomeitem.inputmessage),
      path('addmesgquery',workhomeitem.addmesgquery),
      path('showmessage',workhomeitem.showmessage),
#      path('upload',workhomeitem.upload),
      path('showjsondata',workhomeitem.showjsondata),
      path('showloginfo',workhomeitem.showloginfo),
      path('showloginfobuild',workhomeitem.showloginfo_build),
      path('githandle',workhomeitem.githandle),
      path('browsedocx',workhomeitem.browsedocx),
      ########## [GdbDebugShellApp] #############
      path('getshellappinfo',workhomeitem.getshellappinfo),
       ################## [upload file] ####################
      path('upload/',upload.upload),
      path('showuploadfiles/',upload.showuploadfiles),
      ################## [record question] ####################
      path('editrecordq',workhomeitem.EditRecordQ),
      path('showrecordq',workhomeitem.ShowRecordQ),
      path('recordquestionquery',workhomeitem.recordquestionquery),
]
 
