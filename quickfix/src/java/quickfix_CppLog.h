/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class quickfix_CppLog */

#ifndef _Included_quickfix_CppLog
#define _Included_quickfix_CppLog
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     quickfix_CppLog
 * Method:    clear
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_quickfix_CppLog_clear
  (JNIEnv *, jobject);

/*
 * Class:     quickfix_CppLog
 * Method:    onIncoming
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_quickfix_CppLog_onIncoming
  (JNIEnv *, jobject, jstring);

/*
 * Class:     quickfix_CppLog
 * Method:    onOutgoing
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_quickfix_CppLog_onOutgoing
  (JNIEnv *, jobject, jstring);

/*
 * Class:     quickfix_CppLog
 * Method:    onEvent
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_quickfix_CppLog_onEvent
  (JNIEnv *, jobject, jstring);

#ifdef __cplusplus
}
#endif
#endif
