
#ifndef ANTSAPPLYTRANSFORMSTOPOINTS_H
#define ANTSAPPLYTRANSFORMSTOPOINTS_H

namespace ants
{

int antsApplyTransformsToPoints( std::vector<std::string>, // equivalent to argv of command line parameters to main()
                                 std::ostream* out_stream  // [optional] output stream to write
                                 );

} // namespace ants

#endif // ANTSAPPLYTRANSFORMSTOPOINTS_H
