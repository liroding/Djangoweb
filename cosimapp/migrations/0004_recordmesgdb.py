# Generated by Django 2.0 on 2019-09-19 06:06

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('cosimapp', '0003_notebookdb'),
    ]

    operations = [
        migrations.CreateModel(
            name='recordmesgdb',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('index1', models.TextField()),
                ('index2', models.TextField()),
                ('index3', models.TextField()),
                ('index4', models.TextField()),
                ('index5', models.TextField()),
            ],
        ),
    ]
