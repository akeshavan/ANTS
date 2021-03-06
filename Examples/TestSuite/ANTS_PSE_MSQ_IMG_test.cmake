#####################################################################################
#####################################################################################
set(THIS_TEST_NAME ANTS_PSE_MSQ_IMG)
set(OUTPUT_PREFIX ${CMAKE_BINARY_DIR}/TEST_${THIS_TEST_NAME} )
set(WARP ${OUTPUT_PREFIX}Warp.nii.gz ${OUTPUT_PREFIX}Affine.txt )
set(INVERSEWARP -i ${OUTPUT_PREFIX}Affine.txt ${OUTPUT_PREFIX}InverseWarp.nii.gz )
set(WARP_IMAGE ${CMAKE_BINARY_DIR}/${THIS_TEST_NAME}_warped.nii.gz)
set(INVERSEWARP_IMAGE ${CMAKE_BINARY_DIR}/${THIS_TEST_NAME}_inversewarped.nii.gz)


add_test(NAME ${THIS_TEST_NAME} COMMAND $<TARGET_FILE:ANTS> 2
 -r Gauss[6,0.25]
 -t SyN[1,2,0.1] -i 191x170x90x90x10
 -m PSE[${DEVIL_IMAGE},${ANGEL_IMAGE},${DEVIL_IMAGE},${ANGEL_IMAGE},0.25,0.1,100,0,10]
 -m MSQ[${DEVIL_IMAGE},${ANGEL_IMAGE},1,0.1]
 -o ${OUTPUT_PREFIX}.nii.gz
 --continue-affine 0 --number-of-affine-iterations 0 --geodesic 2)

add_test(NAME ${THIS_TEST_NAME}_WARP COMMAND $<TARGET_FILE:WarpImageMultiTransform> 2
 ${ANGEL_IMAGE} ${WARP_IMAGE} ${WARP} -R ${DEVIL_IMAGE} )
set_property(TEST ${THIS_TEST_NAME}_WARP APPEND PROPERTY DEPENDS ${THIS_TEST_NAME})

add_test(NAME ${THIS_TEST_NAME}_JPG COMMAND $<TARGET_FILE:ConvertToJpg> ${WARP_IMAGE} ${THIS_TEST_NAME}.jpg)
set_property(TEST ${THIS_TEST_NAME}_JPG APPEND PROPERTY DEPENDS ${THIS_TEST_NAME}_WARP)

add_test(NAME ${THIS_TEST_NAME}_WARP_METRIC_0 COMMAND $<TARGET_FILE:MeasureImageSimilarity> 2 0
 ${DEVIL_IMAGE} ${WARP_IMAGE}
 ${OUTPUT_PREFIX}log.txt ${OUTPUT_PREFIX}metric.nii.gz
 0.0116083 0.05)
set_property(TEST ${THIS_TEST_NAME}_WARP_METRIC_0 APPEND PROPERTY DEPENDS ${THIS_TEST_NAME}_WARP)

add_test(NAME ${THIS_TEST_NAME}_WARP_METRIC_1 COMMAND $<TARGET_FILE:MeasureImageSimilarity> 2 1
 ${DEVIL_IMAGE} ${WARP_IMAGE}
 ${OUTPUT_PREFIX}log.txt ${OUTPUT_PREFIX}metric.nii.gz
 0.991658 0.05)
set_property(TEST ${THIS_TEST_NAME}_WARP_METRIC_1 APPEND PROPERTY DEPENDS ${THIS_TEST_NAME}_WARP)

add_test(NAME ${THIS_TEST_NAME}_WARP_METRIC_2 COMMAND $<TARGET_FILE:MeasureImageSimilarity> 2 2
 ${DEVIL_IMAGE} ${WARP_IMAGE}
 ${OUTPUT_PREFIX}log.txt ${OUTPUT_PREFIX}metric.nii.gz
 -0.000710551 0.05)
set_property(TEST ${THIS_TEST_NAME}_WARP_METRIC_2 APPEND PROPERTY DEPENDS ${THIS_TEST_NAME}_WARP)

add_test(NAME ${THIS_TEST_NAME}_INVERSEWARP COMMAND $<TARGET_FILE:WarpImageMultiTransform> 2
 ${DEVIL_IMAGE} ${INVERSEWARP_IMAGE} ${INVERSEWARP} -R ${ANGEL_IMAGE}
 )
set_property(TEST ${THIS_TEST_NAME}_INVERSEWARP APPEND PROPERTY DEPENDS ${THIS_TEST_NAME})

add_test(NAME ${THIS_TEST_NAME}_JPGINV COMMAND $<TARGET_FILE:ConvertToJpg> ${INVERSEWARP_IMAGE} ${THIS_TEST_NAME}INV.jpg)
set_property(TEST ${THIS_TEST_NAME}_JPGINV APPEND PROPERTY DEPENDS ${THIS_TEST_NAME}_INVERSEWARP)

add_test(NAME ${THIS_TEST_NAME}_INVERSEWARP_METRIC_0 COMMAND $<TARGET_FILE:MeasureImageSimilarity> 2 0 ${ANGEL_IMAGE}
 ${INVERSEWARP_IMAGE} ${OUTPUT_PREFIX}log.txt ${OUTPUT_PREFIX}metric.nii.gz
 0.0109054 0.05)
set_property(TEST ${THIS_TEST_NAME}_INVERSEWARP_METRIC_0 APPEND PROPERTY DEPENDS ${THIS_TEST_NAME}_INVERSEWARP)

add_test(NAME ${THIS_TEST_NAME}_INVERSEWARP_METRIC_1 COMMAND $<TARGET_FILE:MeasureImageSimilarity> 2 1 ${ANGEL_IMAGE}
 ${INVERSEWARP_IMAGE} ${OUTPUT_PREFIX}log.txt ${OUTPUT_PREFIX}metric.nii.gz
 0.990979 0.05)
set_property(TEST ${THIS_TEST_NAME}_INVERSEWARP_METRIC_1 APPEND PROPERTY DEPENDS ${THIS_TEST_NAME}_INVERSEWARP)

add_test(NAME ${THIS_TEST_NAME}_INVERSEWARP_METRIC_2 COMMAND $<TARGET_FILE:MeasureImageSimilarity> 2 2 ${ANGEL_IMAGE}
 ${INVERSEWARP_IMAGE} ${OUTPUT_PREFIX}log.txt ${OUTPUT_PREFIX}metric.nii.gz
 -0.000704717 0.05)
set_property(TEST ${THIS_TEST_NAME}_INVERSEWARP_METRIC_2 APPEND PROPERTY DEPENDS ${THIS_TEST_NAME}_INVERSEWARP)

