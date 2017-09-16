
function startTime()
{
var today=new Date()
var h=today.getHours()
var m=today.getMinutes()
var s=today.getSeconds()
// add a zero in front of numbers<10
m=checkTime(m)
s=checkTime(s)
document.getElementById('txt').innerHTML=h+":"+m+":"+s
t=setTimeout('startTime()',500)
}

function checkTime(i)
{
if (i<10) 
  {i="0" + i}
  return i
}


var c1=0
var t1
function timedCount()
{
document.getElementById('txt1').value=c1
c1=c1+1
t1=setTimeout("timedCount()",1000)
}

function stopCount()
{
c1=0;
setTimeout("document.getElementById('txt1').value=0",0);
clearTimeout(t1);
}
