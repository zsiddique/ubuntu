--TEST--
cairo_svg_surface_restrict_to_version() function
--SKIPIF--
<?php 

if(!extension_loaded('cairo_wrapper')) die('skip ');

if(!function_exists('cairo_svg_surface_restrict_to_version')) die('skip not compiled in (CAIRO_HAS_SVG_SURFACE)');

 ?>
--FILE--
<?php
echo 'OK'; // no test case for this function yet
?>
--EXPECT--
OK