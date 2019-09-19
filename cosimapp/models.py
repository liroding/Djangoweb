from django.db import models

# Create your models here.

class login(models.Model):
    user = models.CharField(max_length = 20)
    password = models.CharField(max_length = 20)

    def __str__(self):
        return 'name:'+self.user+','+self.password
class notebookdb(models.Model):
    article_id = models.AutoField(primary_key = True)
    title  = models.TextField()
    author = models.TextField()
    content = models.TextField()
    publish_date = models.DateTimeField(auto_now = True)

    def __str__(self):
        return 'Tile:'+self.title

class recordmesgdb(models.Model):
    index1  = models.TextField()
    index2  = models.TextField()
    index3  = models.TextField()
    index4  = models.TextField()
    index5  = models.TextField()
    def __str__(self):
        return 'index1:'+self.index1

