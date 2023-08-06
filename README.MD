## Make MSYS2 use Windows native ssh-agent or Pageant



This is a simple ssh-agent proxy so MSYS2's ssh/git can use Windows native OpenSSH, [or latest Pageant](https://www.chiark.greenend.org.uk/~sgtatham/putty/wishlist/pageant-named-pipe.html) as its ssh-agent. And since it exposes named pipe as a TCP port it also works on WSL2 with some extra work.

### Background

------
- ssh-agent is used to store the credential and decrypt the private ssh key when necessary, so you DON'T need it(and this program) for an unencrypted ssh key setup.

- Why use Windows ssh-agent vs pageant(or others)?
  While Windows ssh-agent isn't the safest, it's very convenient as it doesn't require repeat password input after initial setup. Also it has a very small memory print(less than 1MB). 

  

### For MSYS2 & Git-Bash

-------

1. Download from release or build your own binary. By default, it listens on port `61000` and creates `C:\MSYS64\tmp\sshwin-msys2.sock` which maps to `/tmp/sshwin-msys2.sock` on stock MSYS2 installation, and use Windows ssh-agent as backend. 

2. In `~/.bashrc` or `~/.profile` , add

   ```
   export SSH_AUTH_SOCK=/tmp/sshwin-msys2.sock
   ```

3. Git-bash should work exactly the same,  adjust `SSH_AUTH_SOCK` accordingly.

4. For use with Pageant, start with`-n pageant.*` . For more info, check full command line options below.

That's it. It's is designed to run in Windows(outside MSYS2) so let it start with Windows if necessary. 

*Can be shared across multiple MSYS2 shells and Git-Bash sessions as long as SSH_AUTH_SOCK points to same disk file.*



### For WSL2

-----

1. In Windows, start the program with `-h 0.0.0.0` option so it will accept connection from WSL2. 
     Add `-f null` to skip MSYS2 part.

     ```
     sshwin-msys2.exe -h 0.0.0.0
     ```

     

2. In WSL2 Linux, install `socat`, add below script to the end of `~/.bashrc` , `socat` will forward the auth socket to host:61000. The script will *restart* `socat` on each login instead of keeping one running long time.

   ```
   MYIP=192.168.1.100  #CHANGE IT
   export SSH_AUTH_SOCK=/tmp/sshwin-msys2.sock
   socatid=$(ps w | grep socat | grep 61000 | cut -d ' ' -f 2)
   if [ $socatid ]; then
     #echo "found"
     kill -9 $socatid > /dev/null 2>&1
   fi
   rm -f $SSH_AUTH_SOCK
   nohup socat UNIX-LISTEN:$SSH_AUTH_SOCK,fork TCP:$MYIP:61000 > /dev/null 2>&1 &
   ```
   
   However things can be tricky when dealing with dynamic IP and/or firewall settings. 



### Command line options

---

```
-f 	filename
	Socket file path, default is "C:\MSYS64\tmp\sshwin-msys2.sock".
	Or set it to "null", prevent the program from creating the disk file(for WSL2).
-h	host
	Default is "127.0.0.1" for localhost traffic only. 
	Can use other IP or "0.0.0.0" for all network interfaces.
-p 	port
	Listen port, default is 61000
-n	pipename
	Windows named pipe used as ssh-agent backend,
	Default is "openssh-ssh-agent" for Windows OpenSSH ssh-agent. 
	For Pageant, set it to "pageant.*" and the program will try to find it.
-t	timeout(in ms)
	Network timeout, default is 60000ms, or 1 minutes.
-r	
	Restart the program.
```





### Note

---

* Make sure Windows OpenSSH is up to date or it might be incompatible with the newer release in MSYS2/WSL2. [Get latest version.](https://github.com/PowerShell/Win32-OpenSSH)



### Tested

----

Windows 10 Build 19044 with Windows OpenSSH 9.1p1, Pageant 0.78 and

* MSYS2(Mintty and ConEmu)
* git-bash
* WSL2 Ubuntu

Untested but should also work on Cygwin.



### Thanks

--------

[Stackoverflow Question](https://stackoverflow.com/questions/23086038/what-mechanism-is-used-by-msys-cygwin-to-emulate-unix-domain-sockets)

[Jimmy-Z implementation in js.](https://gist.github.com/Jimmy-Z/a2f3ef239932110b0febd82c1c5f66f7)
