from django.shortcuts import render

def global_var(request,user):
    print(user)
    return{
            'username':user
    }
