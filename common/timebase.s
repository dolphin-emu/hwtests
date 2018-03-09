.text
.global GetTimebase
.type GetTimebase, @function  # u64 GetTimebase()
GetTimebase:
  mftbu 0
  mftb 4
  mftbu 3
  cmpw cr0,0,3
  beqlr- cr0
  b GetTimebase
