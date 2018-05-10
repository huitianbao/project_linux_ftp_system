#!/bin/sh 
i=1
groupadd class1
while [ $i -le 3]
do
if [ $i -le 9 ] ;then
username=stu0${i}
else
username=stu${i}
fi
useradd $username 
passwd $username
mkdir /home/$username
chown -r $username /home/$username
chgrp -r class1 /home/$username 
i=$(($i+1))
done
  
