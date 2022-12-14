program DynamicHuffman (input, output);
{ Written by J. S. Vitter in Berkeley Pascal and run on a VAX 780 running
  Berkeley UNIX 4.3BSD.  This Pascal program implements Algorithm Lambda,
  described and analyzed in the article by J. S. Vitter entitled
  "Design and Analysis of Dynamic Huffman Codes," JACM, Vol. 34, No. 4,
  October 1987.    There are a couple things to note:
    1. The output is done bit by bit using individual write statements,
       which typically output one byte rather than a bit.  So the "compressed"
       file is typically much larger than the original file, because
       each bit is written as a byte.  This was left
       this way because it makes it easier for the implementor to check
       the algorithm during implementation.  A packing routine for output
       can be added later with an easy modification.
    2. When the program is executed,
       the user is prompted for the following information (in that order):
         a. whether encoding or decoding is desired (reply "en" or "de").
         b. which alphabet to use (reply "keys" for size 96, "ascii" for 256).
         c. input file name
         d. output file name
       Each reply is followed by a carraige return.
       The "keys" alphabet includes the printable ASCII characters, plus
       the newline character.  The programmer would have to modify the code
       to accomodate arbitrary alphabet sizes.  The code for "ascii" alphabet
       uses the fact that in Berkely UNIX, characters are represented by
       8-bit values in two's complement, in the range from -128 to 127.
       Since the program expects alphabet values in the range from 1 to 256,
       some conversion is necessary when characters are read in.
       This is easy to modify for any particular implementation.
    3. Because of a Berkeley Pascal quirk, the program effectively adds
       an end-of-line character to the end of the last line of the file
       if it does not already have one.  The encoded file consists
       of a several line of 0s and 1s (the number of 0s and 1s per line
       is specified by the value of lineLengthLimit),
       terminated with the end-of-line and end-of-file characters.}
 
const
  lineLengthLimit = 78;  { Number of 0s and 1s in encoded file per line}
 
type
  str = packed array [1..80] of char;
  SmallArray = array[1..256] of integer;
  LargeArray = array[1..512] of integer;
  LargeLongArray = array[1..512] of integer;
  AlphabetChoices = (keys, ascii);
  RoleChoices = (encode, decode);
 
var
  lineLength : integer;
  EOLNcode : integer;
  response : char;
  theRole : RoleChoices;
  theAlphabet : AlphabetChoices;
  outputfilename, inputfilename : str;
  n : integer;
  inp, out : text;
 
  stack, alpha, rep : SmallArray;
  M, E, R : integer;
  availBlock, Z : integer;
  parent, rtChild, parity, block, prevBlock, nextBlock,
  first, last : LargeArray;
  weight : LargeLongArray;
 
function FindChild (j, parity : integer) : integer;
var delta, right, gap : integer;
begin
delta := 2 * (first[block[j]] - j) + 1 - parity;
right := rtChild[block[j]]; gap := right - last[block[right]];
if delta <= gap then FindChild := right - delta
else begin
    delta := delta - gap - 1;
    right := first[prevBlock[block[right]]]; gap := right - last[block[right]];
    if delta <= gap then FindChild := right - delta
    else FindChild := first[prevBlock[block[right]]] - delta + gap + 1
    end;
end;
 
function Receive : integer;
var    v : char;
begin
if eoln(inp) then
    begin
    Receive := EOLNcode;
    readln(inp)
    end
else
    begin
    read(inp, v);
    if theAlphabet = keys then Receive := ord(v) - ord(' ') + 2
    else Receive := ord(v) + 129
    end
end;
 
function BitReceive : integer;
var c : char;
begin
lineLength := lineLength + 1;
read(inp, c);
BitReceive := ord(c) - ord('0');
if lineLength = lineLengthLimit then
    begin readln(inp); lineLength:= 0 end
end;
 
procedure EncodeAndTransmit (k : integer);
var i, ii, q, t, root : integer;
begin
q := rep[k]; i := 0;
if q <= M then begin { Encode letter of zero weight }
    q := q - 1;
    if q < 2 * R then t := E + 1
    else begin q := q - R; t := E end;
    for ii := 1 to t do begin
        i := i + 1; stack[i] := q mod 2;
        q := q div 2
        end;
    q := M;
    end;
if M = n then root := n else root := Z;
while q <> root do begin { Traverse up the tree }
    i := i + 1; stack[i] := (first[block[q]] - q + parity[block[q]]) mod 2;
    q := parent[block[q]] - (first[block[q]] - q + 1 - parity[block[q]]) div 2
    end;
 
{ This version of the algorithm outputs each bit in a simple-minded way,
  which in most implentations uses one byte to store each bit.  This can
  be easily fixed for any given implementation.  We have it as is so that
  the implementor can more easily interpret the codes by hand in case there
  are any problems during implementation.  Later, when everything is verified,
  a more efficient output routine that packs the bits together
  can be used instead.   One thing our code does do, however, is limit the
  length of each line in the encoded file to some prespecified number of
  zeros and ones, in case long lines cause a problem for your
  particular Pascal implementation.  These extra end-of-line markers
  are ignored during decoding.  The const lineLengthLimit specifies
  the limit; if it is a negative number no extra end-of-line markers
  are used and this feature is turned off. }
for ii := i downto 1 do begin
    lineLength := lineLength + 1;
    write(out,stack[ii]:1);
    if lineLength = lineLengthLimit then
        begin writeln(out); lineLength:= 0 end;
    end
end;
 
function ReceiveAndDecode : integer;
var j, q : integer;
begin
if M = n then q := n else q := Z; { Set |q| to the root node }
while q > n do  { Traverse down the tree }
    q := FindChild(q, BitReceive);
if q = M then begin  { Decode 0-node }
    q := 0;
    for j := 1 to E do q := 2 * q + BitReceive;
    if q < R then q := 2 * q + BitReceive
    else q := q + R;
    q := q + 1
    end;
ReceiveAndDecode := alpha[q]
end;
 
procedure InterchangeLeaves (e1, e2 : integer);
var temp : integer;
begin
rep[alpha[e1]] := e2; rep[alpha[e2]] := e1;
temp := alpha[e1]; alpha[e1] := alpha[e2]; alpha[e2] := temp
end;
 
procedure Update (k : integer);
var
    q, leafToIncrement, bq, b, oldParent, oldParity, nbq, par, bpar : integer;
    slide : boolean;
 
procedure FindNode;
begin
q := rep[k]; leafToIncrement := 0;
if q <= M then begin { A zero weight becomes positive }
    InterchangeLeaves(q, M);
    if R = 0 then begin
        R := M div 2;
        if R > 0 then E := E - 1
        end;
    M := M - 1; R := R - 1; q := M + 1; bq := block[q];
    if M > 0 then begin
        { New 0-node is node |M|; old 0-node is node |M + 1|;
          new parent of nodes |M| and |M + 1| is node |M + n| }
        block[M] := bq; last[bq] := M; oldParent := parent[bq];
        parent[bq] := M + n; parity[bq] := 1;
        { Create new internal block of zero weight for node |M + n| }
        b := availBlock; availBlock := nextBlock[availBlock];
        prevBlock[b] := bq; nextBlock[b] := nextBlock[bq];
        prevBlock[nextBlock[bq]] := b; nextBlock[bq] := b;
        parent[b] := oldParent; parity[b] := 0; rtChild[b] := q;
        block[M + n] := b; weight[b] := 0;
        first[b] := M + n; last[b] := M + n;
        leafToIncrement := q; q := M + n
        end
    end
else begin { Interchange |q| with the first node in |q|'s block }
    InterchangeLeaves(q, first[block[q]]);
    q := first[block[q]];
    if (q = M + 1) and (M > 0) then
        begin leafToIncrement := q; q := parent[block[q]] end
    end
end;
 
procedure SlideAndIncrement;
begin { |q| is currently the first node in its block }
bq := block[q]; nbq := nextBlock[bq];
par := parent[bq]; oldParent := par; oldParity := parity[bq];
 
if ((q <= n) and (first[nbq] > n) and (weight[nbq] = weight[bq]))
  or ((q > n) and (first[nbq] <= n) and (weight[nbq] = weight[bq] + 1)) then
    begin { Slide |q| over the next block }
    slide := true;
    oldParent := parent[nbq]; oldParity := parity[nbq];
    { Adjust child pointers for next-higher level in tree }
    if par > 0 then begin
        bpar := block[par];
        if rtChild[bpar] = q then rtChild[bpar] := last[nbq]
        else if rtChild[bpar] = first[nbq] then rtChild[bpar] := q
        else rtChild[bpar] := rtChild[bpar] + 1;
        if par <> Z then
            if block[par + 1] <> bpar then
                if rtChild[block[par + 1]] = first[nbq] then
                    rtChild[block[par + 1]] := q
                else if block[rtChild[block[par + 1]]] = nbq then
                    rtChild[block[par + 1]] := rtChild[block[par + 1]] + 1
        end;
    { Adjust parent pointers for block |nbq| }
    parent[nbq] := parent[nbq] -1 +parity[nbq]; parity[nbq] := 1 -parity[nbq];
    nbq := nextBlock[nbq];
    end
else slide := false;
 
if (((q <= n) and (first[nbq] <= n)) or ((q > n) and (first[nbq] > n)))
  and (weight[nbq] = weight[bq] + 1) then
    begin { Merge |q| into the block of weight one higher }
    block[q] := nbq; last[nbq] := q;
    if last[bq] = q then begin  { |q|'s old block disappears }
        nextBlock[prevBlock[bq]] := nextBlock[bq];
        prevBlock[nextBlock[bq]] := prevBlock[bq];
        nextBlock[bq] := availBlock; availBlock := bq
        end
    else begin
        if q > n then rtChild[bq] := FindChild(q - 1, 1);
        if parity[bq] = 0 then parent[bq] := parent[bq] - 1;
        parity[bq] := 1 - parity[bq];
        first[bq] := q - 1
        end
    end
else if last[bq] = q then begin
    if slide then begin { |q|'s block is slid forward in the block list }
        prevBlock[nextBlock[bq]] := prevBlock[bq];
        nextBlock[prevBlock[bq]] := nextBlock[bq];
        prevBlock[bq] := prevBlock[nbq]; nextBlock[bq] := nbq;
        prevBlock[nbq] := bq; nextBlock[prevBlock[bq]] := bq;
        parent[bq] := oldParent; parity[bq] := oldParity
        end;
    weight[bq] := weight[bq] + 1;
    end
else begin { A new block is created for |q| }
    b := availBlock; availBlock := nextBlock[availBlock];
    block[q] := b; first[b] := q; last[b] := q;
    if q > n then begin
        rtChild[b] := rtChild[bq];
        rtChild[bq] := FindChild(q - 1, 1);
        if rtChild[b] = q - 1 then parent[bq] := q
        else if parity[bq] = 0 then parent[bq] := parent[bq] - 1;
        end
    else if parity[bq] = 0 then parent[bq] := parent[bq] - 1;
    first[bq] := q - 1; parity[bq] := 1 - parity[bq];
    { Insert |q|'s block in its proper place in the block list }
    prevBlock[b] := prevBlock[nbq]; nextBlock[b] := nbq;
    prevBlock[nbq] := b; nextBlock[prevBlock[b]] := b;
    weight[b] := weight[bq] + 1;
    parent[b] := oldParent; parity[b] := oldParity
    end;
 
{ Move |q| one level higher in the tree }
if q <= n then q := oldParent else q := par
end; { SlideAndIncrement }
 
begin { Update }
 
{ Set |q| to the node whose weight should increase }
FindNode;
 
while q > 0 do
    { At this point, |q| is the first node in its block.  Increment
      |q|'s weight by 1 and slide if necessary to maintain invariant (*) }
    SlideAndIncrement;
 
{ Finish up some special cases involving the 0-node }
if leafToIncrement <> 0 then begin q := leafToIncrement; SlideAndIncrement end
end;
 
 
procedure Initialize;
var i : integer;
begin
M := 0; E := 0; R := -1; Z := 2 * n - 1;
for i := 1 to n do begin
    M := M + 1; R := R + 1;
    if 2 * R = M then begin E := E + 1; R := 0 end;
    alpha[i] := i; rep[i] := i
    end;
{ Initialize node |n| as the 0-node }
block[n] := 1; prevBlock[1] := 1; nextBlock[1] := 1; weight[1] := 0;
first[1] := n; last[1] := n; parity[1] := 0; parent[1] := 0;
{ Initialize available block list }
availBlock := 2;
for i := availBlock to Z - 1 do
    nextBlock[i] := i + 1;
nextBlock[Z] := 0;
end;
 
procedure ReadString ( var s : str);
var i : integer;
begin
i := 0;
while not eoln do begin i := i + 1; read(s[i]) end;
readln;
while i < 80 do begin i := i + 1; s[i] := ' ' end
end;
 
procedure GetOptions;
begin
writeln('Enter type of coding desired (encode, decode):');
readln(response);
if response = 'e' then theRole := encode else theRole := decode;
writeln('Enter alphabet model (keys, ascii):');
readln(response);
if response = 'k' then theAlphabet := keys else theAlphabet := ascii;
case theAlphabet of
    keys :  { Alphabet size = 96 (printable ASCII characters, plus linefeed) }
        begin n := 96; EOLNcode := 1 end;
    ascii :  { Alphabet size = 256.  Note that 139 is equal to 129 +
               (code for UNIX linefeed character in two's complement) }
        begin n := 256; EOLNcode := 139 end;
    end;
writeln('Enter the input filename: ');
ReadString(inputfilename);
reset(inp, inputfilename);
writeln('Enter the output filename: ');
ReadString(outputfilename);
rewrite(out, outputfilename);
end;
 
procedure PutName (val : integer);
begin
if val = EOLNcode then writeln(out)
else if theAlphabet = keys then write(out, chr(val + ord(' ') - 2))
else write(out, chr(val - 129));
end;
 
procedure Runit ;
var c : integer;
begin
lineLength := 0;
if theRole = encode then
    begin
    while not eof(inp) do begin
        c := Receive;
        EncodeAndTransmit(c);
        Update(c);
        end;
    writeln(out);
    end
else if not eof(inp) then
    while not eoln(inp) do
        begin
        c := ReceiveAndDecode;
        PutName(c);
        Update(c);
        end;
end;
 
begin { Mainline }
GetOptions;
Initialize;
Runit;
end.
 
 
