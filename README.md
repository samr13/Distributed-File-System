Distributed file system for reliable and secure file storage with traffic optimization using C

start server by:
./dfs1  DFS1   10001   &   
./dfs2  DFS2   10002   &   
./dfs3  DFS3   10003   &   
./dfs4  DFS4   10004   &  

start client by:
1) ./dfc Bob xad345 
2) ./dfc => takes username & password from dfc.conf

list or 
list sub1/

put sam.txt or
put sam.txt sub/


get sam.txt or
get sam.txt sub/

Creates encrypted file as <file>_enc in client folder and pass encrypted file.
Creates decrypted file in folder DFS1/<username>/dec/<file>_d and save splits file in DFS1/<username>/<file>
