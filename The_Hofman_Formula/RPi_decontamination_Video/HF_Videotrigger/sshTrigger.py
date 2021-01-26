import paramiko
from sshTriggerCfg import Settings as cfg
from time import sleep

user = cfg["user"]
pwd = cfg["pwd"]
ip = cfg["ip"]
cmd = cfg["cmd"]

ssh = paramiko.SSHClient()


def connect_shh():
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(ip, 22, user, pwd)


def trigger_script():
    connect_shh()
    # stdin, stdout, stderr = 
    _, _, stderr = ssh.exec_command(cmd)
    print(stderr)


if __name__ == "__main__":
    # stuff only to run when not called via 'import' here
    connect_shh()
