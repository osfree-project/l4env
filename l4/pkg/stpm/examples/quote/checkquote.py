#!/usr/bin/python

"Check the signature of TPM_quote"

import sys
import string
from Crypto.PublicKey import RSA
from Crypto.Hash import SHA
import struct

import re

DEBUG=0

def debug(value):
    if DEBUG:
        print value

def read_pubfile(filename):
    lines=open(filename).readlines()
    line="".join(map(string.strip,lines))
    pos=line.index("Exponent")
    assert pos>=0
    
    modstr=line[:pos]

    rm=re.match("Modulus .*?:(.+)",modstr)
    assert rm,"Modulus doesnot match!"
    mod=long(rm.group(1).replace(":",""),16)

    
    expstr=line[pos:]
    rm=re.match("Exponent: (\d+)\s+\(.*\)",expstr)
    assert rm,"Exponent doesnot match!"
    exp=long(rm.group(1))
    return (mod,exp)

def read_quote_output(value,logname=None):    
    """use the output from quote_stdout and
    return a list of pcrs and the signature"""
    # cut the name away
    lines=value.split("\n")
    m=map(lambda x:x.split("|"),lines)
    if logname:
        m=filter(lambda x:x[0].find(logname)>=0,m)
    m=map(lambda x:x[-1],m)        
    value="\n".join(m)

    # search version in string
    rm=re.search("TPM version: (\d+)\.(\d+)\.(\d+)\.(\d+)",value)
    assert rm, "version not found in %s"%repr(value) 
    version=map(int,rm.groups())

    # search a name string    
    rm=re.search("app loading: (.+)",value)
    if rm:
        assert rm, "name not found"    
        name=rm.group(1)
    else:
        name="<unknown>"

    # search a hash value
    # rm=re.search("sha1\[:\d+\]: (.+)",value)
    
    # split input
    v=re.split("pcrcomposite \(.*\):",value)
    assert len(v)>=2
    v=re.split("signature \(.*\):",v[-1])
    assert len(v)==2

    # collect pcr values
    s=re.sub("\s","",v[0])
    pcrs=re.split("PCR-\d+:",s)
    pcrs=map(lambda x:long(x,16),pcrs[1:])

    v=re.split("signature end",v[1])
    assert len(v)==2
    
    # calc signature
    sig=re.sub("\s","",v[0])
    sig=long(sig,16)
    debug("sig: %s"%hex(sig))
    
    return (version,pcrs,sig,name,hash)

def calc_hash(pcrs):
    s=""
    s+=struct.pack("!H",2)
    s+=struct.pack("!H",0xffff)
    s+=struct.pack("!I",len(pcrs)*20)
    for pcr in pcrs:
        s+=("\x00"*20+long2bin(pcr))[-20:]
    hash=SHA.new()
    hash.update(s)
#    print len(s),hex(bin2long(s))
    return hash.digest()

def quote_hash(pcr_hash,external_hash,version):
    s=""
    s+=apply(struct.pack,["!BBBB"]+version)
    s+="QUOT"
    s+=pcr_hash
    s+=external_hash
    
    hash=SHA.new()
    hash=SHA.new()
    hash.update(s)
    return hash.digest()
   
    
def bin2long(value):
    return reduce(lambda x,y:(x<<8)+ord(y),value,0l)

def long2bin(value):
    res=""
    while value>0:
        res=chr(value%256)+res
        value>>=8
    return res


def checkquote(keyfile,params,nounce):
    
    version,pcrs,sig,name,hash=params
    # create key
    pub=read_pubfile(keyfile)
    key=RSA.construct(pub)
    
    # encrypt signature
    encrypt=long2bin(key.encrypt(sig,None)[0])

    # check the results
    assert encrypt[1:256-37]=="\xff"*(256-38), "Is this the right key?\n%s"%(hex(bin2long(encrypt)))

    # calc other hashes
    hash=SHA.new(nounce).digest()
    myhash=calc_hash(pcrs)
    qhash=quote_hash(myhash,hash,version)

    debug("NounceHash:\t%s"%hex(bin2long(hash)))
    debug("MyHash:    \t%s"%hex(bin2long(myhash)))
    debug("QHash:     \t%s"%hex(bin2long(qhash)))
    debug("EncHash:   \t%s"%hex(bin2long(encrypt[-20:])))
    
    # for what are these 16 Bytes?
    debug("Unknown: \t%s"%repr(encrypt[-36:-20]))

    if qhash==encrypt[-20:]:
        return 0
    else:
        return 1

    

def main():
    if len(sys.argv)<2:
        print "Usage: %s pubkeyfile"%(sys.argv[0])
        return
    v=read_quote_output(sys.stdin.read())
    res=checkquote(sys.argv[1],v,"nounce")
    if res:
        print "signature does not match"
    else:
        print "signature match"


if __name__=="__main__":
    main()
