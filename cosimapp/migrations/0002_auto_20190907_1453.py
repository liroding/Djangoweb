# Generated by Django 2.0 on 2019-09-07 14:53

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('cosimapp', '0001_initial'),
    ]

    operations = [
        migrations.AlterField(
            model_name='login',
            name='password',
            field=models.CharField(max_length=20),
        ),
        migrations.AlterField(
            model_name='login',
            name='user',
            field=models.CharField(max_length=20),
        ),
    ]
