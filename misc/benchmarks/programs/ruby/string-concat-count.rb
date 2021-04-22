str = """The god of love,
That sits above,
And knows me, and knows me,
How pitiful I deserve,--
I mean in singing; but in loving, Leander the good
swimmer, Troilus the first employer of panders, and
a whole bookful of these quondam carpet-mangers,
whose names yet run smoothly in the even road of a
blank verse, why, they were never so truly turned
over and over as my poor self in love. Marry, I
cannot show it in rhyme; I have tried: I can find
out no rhyme to 'lady' but 'baby,' an innocent
rhyme; for 'scorn,' 'horn,' a hard rhyme; for,
'school,' 'fool,' a babbling rhyme; very ominous
endings: no, I was not born under a rhyming planet,
nor I cannot woo in festival terms.

Enter BEATRICE
Sweet Beatrice, wouldst thou come when I called thee?"""

for i in 0..7 do
    str = str + str
end

count = 0
str.length.times { |i| 
   if str[i] == ' '; then
        count = count + 1
   end
}


puts(count)