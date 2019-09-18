# -*_ coding:utf-8 _*
class GlobalVarBuf():
    def __init__():
        global _global_dict
        _global_dict = {}
    def set_value(key,value):
        _global_dict[key] = value
#        print('<1>'+key+','+ value)
    def get_value(key,defvalue=None):
        try:
#            print('<2>'+_global_dict[key])
            return _global_dict[key]
        except KeyError:
            return defvalue
