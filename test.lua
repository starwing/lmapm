-- test mapm library
-- cc: cc='lua52'

-- reuse test code from bc library
bc=require"mapm"
mapm=bc
bc.version = bc._VERSION
bc.number = bc.new
bc.digits = bc.setprec
bc.compare = bc.cmp


------------------------------------------------------------------------------
print(bc.version)

------------------------------------------------------------------------------
print""
print"Pi algorithm of order 4"

bc.digits(79)
PI=bc.number"3.1415926535897932384626433832795028841971693993751058209749445923078164062862090"

-- http://pauillac.inria.fr/algo/bsolve/constant/pi/pi.html
function A2()
 local x=bc.sqrt(2)
 local p=2+x
 local y=bc.sqrt(x)
 print(-1,p)
 x=(y+1/y)/2
 p=p*(x+1)/(y+1)
 print(0,p)
 for i=1,20 do
  local P=p
  local t=bc.sqrt(x)
  y=(y*t+1/t)/(y+1)
  x=(t+1/t)/2
  p=p*(x+1)/(y+1)
  print(i,p)
  if p==P then break end
 end
 return p
end

p=A2()
print("exact",PI)
print("-",bc.abs(PI-p))

------------------------------------------------------------------------------
print""
print"Square root of 2"

bc.digits(65)
function mysqrt(x)
 local y,z=x,x
 repeat z,y=y,(y+x/y)/2 until z==y
 return y
end

print("f math",math.sqrt(2))
print("f mine",mysqrt(2))
a=bc.sqrt(2) print("B sqrt",a)
b=mysqrt(bc.number(2)) print("B mine",b)
R=bc.number"1.414213562373095048801688724209698078569671875376948073176679737990732478462107038850387534327641573"
print("exact",R)
print(a==b,a<b,a>b,bc.compare(a,b))

------------------------------------------------------------------------------
print""
print"Fibonacci numbers as digits in fraction"

x=99989999
bc.digits(68)
a=bc.div(1,x)
s=bc.tostring(a)
print("1/"..x.." =")
print("",s)
s=string.gsub(s,"0(%d%d%d)"," %1")
print("",s)

------------------------------------------------------------------------------
print""
print"Factorials"

function factorial(n,f)
 for i=2,n do f=f*i end
 return f
end

one=bc.number(1)
for i=1,30 do
 print(i,factorial(i,1),bc.tostring(factorial(i,one),0))
end

------------------------------------------------------------------------------
print""
print"Comparisons"

bc.digits(4)
a=bc.div(4,2)
b=bc.number(1)
print("a","b","a==b","a<b","a>b","bc.compare(a,b)")
print(a,b,a==b,a<b,a>b,bc.compare(a,b))
b=b+1
print(a,b,a==b,a<b,a>b,bc.compare(a,b))
b=b+1
print(a,b,a==b,a<b,a>b,bc.compare(a,b))

------------------------------------------------------------------------------
print""
print"Is exp(pi*sqrt(163)) an integer?"

for n=1,40 do
 mapm.digits(n)
 pi=4*mapm.atan(1)
 a=mapm.exp(pi*mapm.sqrt(163))
 print(n,mapm.isint(a),a)
end
n=40
a=math.exp(math.pi*math.sqrt(163))
print("f",mapm.isint(a),string.format("%."..n.."f",a))

------------------------------------------------------------------------------
print""
print"Gregory series"

-- http://mathworld.wolfram.com/GregorySeries.html
bc.digits(78)
a=bc.number(0)
for i=1,1000 do
 a=a+(-1)^i*bc.inv(2*i-1)
end
a=-4*a
print(a)
print(PI)

a=mapm.tostring(a)
b=mapm.tostring(PI)
s=""
for i=1,string.len(b) do
 local A=string.sub(a,i,i)
 local B=string.sub(b,i,i)
 if A==B then s=s.."-" else s=s.."x" end
end
print(s)

------------------------------------------------------------------------------
print""
print(bc.version)

-- eof
