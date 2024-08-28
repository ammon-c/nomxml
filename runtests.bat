xmldump testdata\01.03.Schleswig2Kropp.kml > 01.03.Schleswig2Kropp.kml.out
xmldump testdata\balloon-image-rel.kml > balloon-image-rel.kml.out
xmldump testdata\Barques2CalaTuent.kml > Barques2CalaTuent.kml.out
xmldump testdata\Drvenik2Trog.kml > Drvenik2Trog.kml.out
xmldump testdata\Google_Earth_Community.kml > Google_Earth_Community.kml.out
xmldump testdata\placemark.kml > placemark.kml.out
xmldump testdata\radio-folder.kml > radio-folder.kml.out
xmldump testdata\books.xml > books.xml.out
xmldump testdata\cd_catalog.xml > cd_catalog.xml.out
xmldump testdata\food.xml > food.xml.out
xmldump testdata\note.xml > note.xml.out
xmldump testdata\note2.xml > note2.xml.out
xmldump testdata\note_error.xml > note_error.xml.out
xmldump testdata\personal.xml > personal.xml.out
xmldump testdata\simple.xml > simple.xml.out
xmldump testdata\airports.gml > airports.gml.out
xmldump testdata\os-mastermap-itn-layer-sample-data.gml > os-mastermap-itn-layer-sample-data.gml.out
xmldump testdata\os-mastermap-itn-urban-paths-layer-sample-data.gml > os-mastermap-itn-urban-paths-layer-sample-data.gml.out
xmldump testdata\os-mastermap-water-network-sample-data.gml > os-mastermap-water-network-sample-data.gml.out
xmldump testdata\places.gml > places.gml.out

rem #### Skip the largest XML files to speed up testing ####
goto skip_large

xmldump testdata\BentonTaxlots.kml > BentonTaxlots.kml.out
xmldump testdata\BKG_NAS_Peine.xml > BKG_NAS_Peine.xml.out
xmldump testdata\sample.gml > sample.gml.out
xmldump testdata\lidar.kml > lidar.kml.out
xmldump testdata\sx9090.gml > sx9090.gml.out
xmldump testdata\sx9090_2.gml > sx9090_2.gml.out
xmldump testdata\sx9090_3.gml > sx9090_3.gml.out
xmldump testdata\sx99sw.gml > sx99sw.gml.out

:skip_large

