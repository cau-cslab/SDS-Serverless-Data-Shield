from sdsmemtools import MemView
cnt = "1000"
a = MemView(cnt)
print(a.bsize())
a = MemView(str(int(cnt)+1))
print(a.bsize())