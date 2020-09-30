1.How I finish th program

  (1)After // Loop for handling connections(Line 96 ~ 110)
     First,open account_list.
     Then initialize 2 fd_set,one for reading,and another for working.
     Set svr.listen_fd in readset and workset so that we can listen new connections.
     And we set flock and an array "alreadylock" here.We will need these later.

  (2)// TODO: Add IO multiplexing(Line 112 ~ 145)
     Use select to find which fd is ready to do something.
     In check new connection,if we have a new connection,we should set it into workset.
     If it is not connection,we are going to see whether it is in set(ready to do something).

  (3)#ifdef READ_SERVER
     In order to make sure the account we want to check won't be changed when we check it,we set a RDLCK first.
     Here we need to use "alreadylock" to check if the account is already connected to the clients who use the same server and want to write.
     (Reading is not affected.)

     /*RDLCK and WRLCK is dealt with the multi-servers condition.*/

     If we fail to set the RDLCK,then somone is changing this account.
     After we check the account,UNLCK the account,remove the fd from the workset,and the connection is closed.

  (4)#else(WRITE_SERVER)
     Just like READ_SERVER,but we need to consider more.
     First it's whether the fd is ready to do some action.
     If it's not ready,we should check which account the client want to change.
     In order to make sure the account we want to change won't be changed when we change it,we set a WRLCK first.
     Here we need to use "alreadylock" to check if the account is already connected to the clients who use the same server.

     /*RDLCK and WRLCK is dealt with the multi-servers condition.*/

     If we fail to set the WRLCK or the account is already locked,then somone is changing this account,and the connection is closed.
     If we succeed to set the WRLCK,then we have the permission to do some action to this account,but we need to wait the client give the order.
     Because writing is in two steps,so we need to store account_index in another place.I use requestP[i].item.
     After we check the account,UNLCK the account,remove the fd from the workset,and the connection is closed.

  (5)#endif(Line 287 ~ 295)
     Just close account_list and free requestP's memeory here.

2.Special Thanks to:

  (1)All TAs who answer my questions and help me solve my problem I encountered,no matter via e-mail or at TA hour.

  (2)B06902132,my classmate who has already passed this course last year.
     I asked him some questions when I was writing the 1 on 1 read_server and write_server part and file lock.
     He gave me some ideas and some concepts that helped me know how these 2 servers work.

  (3)B06902124,my another classmate who has already passed this course last year.
     I asked him some questions when I have problems dealing with IO multiplexing(using select function) and check new connetion.
     He helped me solve these 2 problems so that my server.c can work correctly.
     (I refered to the way he dealt with the new connetion,but I knew how it works first,and wrote my program later.)