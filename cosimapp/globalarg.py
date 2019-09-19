from django.shortcuts import render

from cosimapp.globalvar.GlobalVar import GlobalVarBuf  

GlobalVarBuf.__init__()
def global_var(request):
    username = GlobalVarBuf.get_value('username')
    return{
            'username':username
    }
