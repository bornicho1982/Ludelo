// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL
//
// Mac App Store only: thin Obj-C++ wrapper around `+[SKStoreReviewController requestReview]`.
// The class method is deprecated in macOS 14 but is the only stable path under AppKit/Qt.
// Defensive guards (@available, NSClassFromString, respondsToSelector, @try/@catch) downgrade
// any future runtime failure to a log + early return.

#ifdef CHIAKI_IS_MAC_APPSTORE

#import <Foundation/Foundation.h>
#import <StoreKit/StoreKit.h>

#include <QLoggingCategory>
#include <QString>

Q_DECLARE_LOGGING_CATEGORY(chiakiAppReview)

void LudeloAppReview_RequestReview()
{
    if (@available(macOS 10.14, *)) {
        Class controllerClass = NSClassFromString(@"SKStoreReviewController");
        if (controllerClass == nil || ![controllerClass respondsToSelector:@selector(requestReview)]) {
            qCWarning(chiakiAppReview) << "SKStoreReviewController unavailable at runtime";
            return;
        }
        @try {
            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wdeprecated-declarations"
            [SKStoreReviewController requestReview];
            #pragma clang diagnostic pop
        } @catch (NSException *exception) {
            qCCritical(chiakiAppReview).nospace()
                << "SKStoreReviewController threw: "
                << QString::fromNSString(exception.name) << " — "
                << QString::fromNSString(exception.reason);
        }
    }
}

#endif // CHIAKI_IS_MAC_APPSTORE

