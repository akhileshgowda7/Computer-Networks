import netcat

senderEmail=str(input("Enter the sender email address\t"))
receiverEmail=input("Enter the receiver email address\t")
Subject=input("Enter the subject\t")
fileName=input("Enter the filename with contents\t")


nc=netcat.Netcat()
nc.connect("mail-relay.iu.edu",25)
raw_data = nc.read(1024)
nc.write("HELO test\r\n")
print(nc.read(1024))
nc.write(f"MAIL FROM:{senderEmail}\r\n")
print(nc.read(1024))
nc.write(f"RCPT TO:{receiverEmail}\r\n")
print(nc.read(1024))
data_cmd = f"DATA\r\n"
nc.write(data_cmd)
print(data_cmd)
print(nc.read(1024))
subject_cmd=f"subject:{Subject}\r\n"
#nc.write(subject_cmd)

#print(nc.read(1024))
with open(fileName) as f:
    contents=f.read()
    end_content="\r\n.\r\n"
    nc.write(f"{subject_cmd}{contents}{end_content}")
    print(subject_cmd)
    print(contents)
    print(end_content)
print(nc.read(1024))

nc.close()
