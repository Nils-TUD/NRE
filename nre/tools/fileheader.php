<?php
$header = <<<EOF
/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */
EOF;

function getFiles($dir) {
    $files = array();
    $d = opendir($dir);
    if($d) {
        while($f = readdir($d)) {
            if($f == '.' || $f == '..' || $f == '.svn' || $f == '.git') {
                continue;
            }
            if(is_dir($dir.'/'.$f)) {
                $files = array_merge($files,getFiles($dir.'/'.$f));
            }
            else {
                $files[] = $dir.'/'.$f;
            }
        }
        closedir($d);
    }
    return $files;
}

foreach(getFiles('.') as $file) {
    if(preg_match('/\.(c|cc|h|s)$/',$file)) {
        $s = file_get_contents($file);
        if(preg_match('/^\/(\*.+?\*\/)/s',$s,$m)) {
            if(preg_match('/(Vancouver|dlmalloc|Michal|bk@vmmon.org)/',$m[1]))
                continue;
            $s = preg_replace('/^\/\*\*?.+?\*\//s',$header,$s);
        }
        else
            $s = $header."\n\n".$s;
        file_put_contents($file,$s);
    }
}
?>
