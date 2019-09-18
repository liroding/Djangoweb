from django.shortcuts import render

from cosimapp.globalvar.GlobalVar import GlobalVarBuf  

GlobalVarBuf.__init__()
def global_var(request):
#    GlobalVarBuf.__init__()
    username = GlobalVarBuf.get_value('username')
    print(username)
    return{
            'username':username
    }
