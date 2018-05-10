#!/bin/sh 
i=1
while [ $i -le 3 ]
do
if [ $i -le 9 ] ;then
username=stu0${i}
else
username=stu${i}
fi
cd /
cd home
rmdir $username
userdel $username 
i=$(($i+1))
done

groupdel class1

  
