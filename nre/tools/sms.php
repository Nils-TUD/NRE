<?php
error_reporting(E_ALL);
if($argc < 2) {
    printf("Usage: %s <file>\n",$argv[0]);
    return 1;
}

$locks = array();
$matches = array();
$content = file_get_contents($argv[1]);
preg_match_all('/\\[\\s*(\\d+)\\] --- EC:([a-f0-9]+) SM:([a-f0-9]+) (UP|DOWN) ---/',$content,$matches);

foreach($matches[0] as $k => $v) {
    $cpu = $matches[1][$k];
    $ec = $matches[2][$k];
    $sm = $matches[3][$k];
    $type = $matches[4][$k];
    
    if(!isset($locks[$sm]))
        $locks[$sm] = array(false,0,0);
    if($type == 'DOWN') {
        $locks[$sm][0] = true;
        $locks[$sm][1] = $cpu;
        $locks[$sm][2] = $ec;
    }
    else
        $locks[$sm][0] = false;
}

foreach($locks as $sm => $data) {
    if($data[0])
        echo $sm.' => CPU:'.$data[1].', EC:'.$data[2]."\n";
}
?>
