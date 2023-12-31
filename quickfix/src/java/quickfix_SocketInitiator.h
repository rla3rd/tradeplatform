/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class quickfix_SocketInitiator */

#ifndef _Included_quickfix_SocketInitiator
#define _Included_quickfix_SocketInitiator
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     quickfix_SocketInitiator
 * Method:    create
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_quickfix_SocketInitiator_create
  (JNIEnv *, jobject);

/*
 * Class:     quickfix_SocketInitiator
 * Method:    destroy
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_quickfix_SocketInitiator_destroy
  (JNIEnv *, jobject);

/*
 * Class:     quickfix_SocketInitiator
 * Method:    doStart
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_quickfix_SocketInitiator_doStart
  (JNIEnv *, jobject);

/*
 * Class:     quickfix_SocketInitiator
 * Method:    doBlock
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_quickfix_SocketInitiator_doBlock
  (JNIEnv *, jobject);

/*
 * Class:     quickfix_SocketInitiator
 * Method:    doStop
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_quickfix_SocketInitiator_doStop__
  (JNIEnv *, jobject);

/*
 * Class:     quickfix_SocketInitiator
 * Method:    doStop
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_quickfix_SocketInitiator_doStop__Z
  (JNIEnv *, jobject, jboolean);

/*
 * Class:     quickfix_SocketInitiator
 * Method:    doIsLoggedOn
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_quickfix_SocketInitiator_doIsLoggedOn
  (JNIEnv *, jobject);

/*
 * Class:     quickfix_SocketInitiator
 * Method:    doGetSessions
 * Signature: ()Ljava/util/ArrayList;
 */
JNIEXPORT jobject JNICALL Java_quickfix_SocketInitiator_doGetSessions
  (JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif
#endif
