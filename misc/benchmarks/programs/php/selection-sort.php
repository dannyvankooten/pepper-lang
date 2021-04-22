<?php

function selsort($arr) {
    for($i=0; $i < count($arr); $i++) {
        $idx_min = $i;

        for ($j=$i+1; $j < count($arr); $j++) {
            if ($arr[$j] < $arr[$idx_min]) {
                $idx_min = $j;
            }
        }

        $tmp = $arr[$i];
        $arr[$i] = $arr[$idx_min];
        $arr[$idx_min] = $tmp;
    }

    return $arr;
}

$arr = file_get_contents("numbers.txt");
$arr = explode(",", $arr);
$arr = selsort($arr);
echo "$arr[0] - $arr[4999]";
