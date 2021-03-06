/*
 Copyright (c) 2011-present, salesforce.com, inc. All rights reserved.
 
 Redistribution and use of this software in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:
  * Redistributions of source code must retain the above copyright notice, this list of conditions
    and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice, this list of
    conditions and the following disclaimer in the documentation and/or other materials provided
    with the distribution.
  * Neither the name of salesforce.com, inc. nor the names of its contributors may be used to
    endorse or promote products derived from this software without specific prior written
    permission of salesforce.com, inc.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import <Foundation/Foundation.h>
#import "SFRestRequest.h"
#import "SFSObjectTree.h"
#import "SFUserAccount.h"

NS_ASSUME_NONNULL_BEGIN

/*
 * Domain used for errors reported by the rest API (non HTTP errors)
 * (for example, passing an invalid SOQL string when doing a query)
 */
extern NSString* const kSFRestErrorDomain;
/*
 * Error code used for all rest API errors (non HTTP errors)
 * (for example, passing an invalid SOQL string when doing a query)
 */
extern NSInteger const kSFRestErrorCode;

/*
 * Default API version (currently "v42.0")
 * You can override this by using setApiVersion:
 */
extern NSString* const kSFRestDefaultAPIVersion;

/*
 * Misc keys appearing in requests
 */
extern NSString* const kSFRestIfUnmodifiedSince;

/**
 Main class used to issue REST requests to the standard Force.com REST API.
 
 See the [Force.com REST API Developer's Guide](http://www.salesforce.com/us/developer/docs/api_rest/index.htm)
 for more information regarding the Force.com REST API.

 ## Initialization
 
 This class is a singleton, and can be accessed by referencing [SFRestAPI sharedInstance].  It relies
 upon the shared credentials managed by SFAccountManager, for forming up and sending authenticated
 REST requests.
 
 ## Sending requests

 Sending a request is done using `send:delegate:`.
 The class sending the request has to conform to the protocol `SFRestDelegate`.
 
 A request can be obtained in two different ways:

 - by calling the appropriate `requestFor[...]` method

 - by building the `SFRestRequest` manually
 
 Note: If you opt to build an SFRestRequest manually, you should be aware that
 send:delegate: expects that if the request.path does not begin with the
 request.endpoint prefix, it will add the request.endpoint prefix 
 (kSFDefaultRestEndpoint by default) to the request path.
  
 For example, this sample code calls the `requestForDescribeWithObjectType:` method to return
 information about the Account object.

    - (void)describeAccount {
        SFRestRequest *request = [[SFRestAPI sharedInstance]
                                  requestForDescribeWithObjectType:@"Account"];
        [[SFRestAPI sharedInstance] send:request delegate:self];
    }
 
    #pragma mark - SFRestDelegate
 
    - (void)request:(SFRestRequest *)request didLoadResponse:(id)dataResponse rawResponse:(NSURLResponse *)rawResponse {
        NSDictionary *dict = (NSDictionary *)dataResponse;
        NSArray *fields = (NSArray *)[dict objectForKey:@"fields"];
        // ...
    }
 
    - (void)request:(SFRestRequest*)request didFailLoadWithError:(NSError *)error rawResponse:(NSURLResponse *)rawResponse {
        // handle error
    }
 
    - (void)requestDidCancelLoad:(SFRestRequest *)request {
        // handle error
    }

    - (void)requestDidTimeout:(SFRestRequest *)request {
        // handle error
    }
 
 ## Error handling
 
 When sending a `SFRestRequest`, you may encounter one of these errors:

 - The request parameters could be invalid (for instance, passing `nil` to the `requestForQuery:`,
 or trying to update a non-existent object).
 In this case, `request:didFailLoadWithError:` is called on the `SFRestDelegate`.
 The error passed will have an error domain of `kSFRestErrorDomain`
 
 - The oauth access token (session ID) managed by SFAccountManager could have expired.
 In this case, the framework tries to acquire another access token and re-issue
 the `SFRestRequest`. This is all done transparently and the appropriate delegate method
 is called once the second `SFRestRequest` returns. 

 - Requesting a new access token (session ID) could fail (if the access token has expired
 and the OAuth refresh token is invalid).
 In this case, `request:didFailLoadWithError:` will be called on the `SFRestDelegate`.
 The error passed will have an error domain of `kSFOAuthErrorDomain`.
 Note that this is a very rare case.

 - The underlying HTTP request could fail (Salesforce server is innaccessible...)
 In this case, `request:didFailLoadWithError:` is called on the `SFRestDelegate`.
 The error passed will be a standard `RestKit` error with an error domain of `RKRestKitErrorDomain`. 

 */
@interface SFRestAPI : NSObject

/**
 * The REST API version used for all the calls.
 * The default value is `kSFRestDefaultAPIVersion` (currently "v42.0")
 */
@property (nonatomic, strong) NSString *apiVersion;

/**
 * The user associated with this instance of SFRestAPI.
 */
@property (nonatomic, strong, readonly) SFUserAccount *user;

/**
 * Returns the singleton instance of `SFRestAPI` associated with the current user.
 */
+ (SFRestAPI *)sharedInstance;

/**
 * Returns the singleton instance of `SFRestAPI` associated with the specified user.
 */
+ (nullable SFRestAPI *)sharedInstanceWithUser:(nonnull SFUserAccount *)user;

/**
 * Specifies whether the current execution is a test run or not.
 @param isTestRun YES if this is a test run
 */
+ (void) setIsTestRun:(BOOL)isTestRun;

/**
 * Specifies whether the current execution is a test run or not.
 */
+ (BOOL) getIsTestRun;

/**
 * Clean up due to host change or logout.
 */
- (void)cleanup;

/** 
 * Cancel all requests that are waiting to be excecuted.
 */
- (void)cancelAllRequests;

/**
 * Sends a REST request to the Salesforce server and invokes the appropriate delegate method.
 * @param request the SFRestRequest to be sent
 * @param delegate the delegate object used when the response from the server is returned. 
 * This overwrites the delegate property of the request.
 */
- (void)send:(SFRestRequest *)request delegate:(nullable id<SFRestDelegate>)delegate;

///---------------------------------------------------------------------------------------
/// @name SFRestRequest factory methods
///---------------------------------------------------------------------------------------

/**
 * Returns an `SFRestRequest` which gets information aassociated with the current user.
 * @see https://help.salesforce.com/articleView?id=remoteaccess_using_userinfo_endpoint.htm
 */
- (SFRestRequest *)requestForUserInfo;

/**
 * Returns an `SFRestRequest` which lists summary information about each
 * Salesforce.com version currently available, including the version, 
 * label, and a link to each version's root.
 * @see http://www.salesforce.com/us/developer/docs/api_rest/Content/resources_versions.htm
 */
- (SFRestRequest *)requestForVersions;

/**
 * Returns an `SFRestRequest` which lists available resources for the
 * client's API version, including resource name and URI.
 * @see Rest API link: http://www.salesforce.com/us/developer/docs/api_rest/Content/resources_discoveryresource.htm
 */
- (SFRestRequest *)requestForResources;

/**
 * Returns an `SFRestRequest` which lists the available objects and their
 * metadata for your organization's data.
 * @see http://www.salesforce.com/us/developer/docs/api_rest/Content/resources_describeGlobal.htm
 */
- (SFRestRequest *)requestForDescribeGlobal;

/**
 * Returns an `SFRestRequest` which describes the individual metadata for the
 * specified object.
 * @param objectType object type; for example, "Account"
 * @see http://www.salesforce.com/us/developer/docs/api_rest/Content/resources_sobject_basic_info.htm
 */
- (SFRestRequest *)requestForMetadataWithObjectType:(NSString *)objectType;

/**
 * Returns an `SFRestRequest` which completely describes the individual metadata
 * at all levels for the 
 * specified object.
 * @param objectType object type; for example, "Account"
 * @see http://www.salesforce.com/us/developer/docs/api_rest/Content/resources_sobject_describe.htm
 */
- (SFRestRequest *)requestForDescribeWithObjectType:(NSString *)objectType;

/**
 * Returns an `SFRestRequest` which provides layout data for the specified object and layout type.
 *
 * @param objectType Object type. For example, "Account".
 * @param layoutType Layout type. Could be "Full" or "Compact". Default is "Full".
 * @see https://developer.salesforce.com/docs/atlas.en-us.uiapi.meta/uiapi/ui_api_resources_record_layout.htm
 */
- (SFRestRequest *)requestForLayoutWithObjectType:(nonnull NSString *)objectType layoutType:(nullable NSString *)layoutType;

/**
 * Returns an `SFRestRequest` which retrieves field values for a record of the given type.
 * @param objectType object type; for example, "Account"
 * @param objectId the record's object ID
 * @param fieldList comma-separated list of fields for which 
 *               to return values; for example, "Name,Industry,TickerSymbol".
 *               Pass nil to retrieve all the fields.
 * @see http://www.salesforce.com/us/developer/docs/api_rest/Content/resources_sobject_retrieve.htm
 */
- (SFRestRequest *)requestForRetrieveWithObjectType:(NSString *)objectType
                                           objectId:(NSString *)objectId 
                                          fieldList:(nullable NSString *)fieldList;

/**
 * Returns an `SFRestRequest` which creates a new record of the given type.
 * @param objectType object type; for example, "Account"
 * @param fields an NSDictionary containing initial field names and values for 
 *               the record, for example, {Name: "salesforce.com", TickerSymbol: 
 *               "CRM"}
 * @see http://www.salesforce.com/us/developer/docs/api_rest/Content/resources_sobject_retrieve.htm
 */
- (SFRestRequest *)requestForCreateWithObjectType:(NSString *)objectType 
                                           fields:(nullable NSDictionary<NSString*, id> *)fields;

/**
 * Returns an `SFRestRequest` which creates or updates record of the given type, based on the 
 * given external ID.
 * @param objectType object type; for example, "Account"
 * @param externalIdField external ID field name; for example, "accountMaster__c"
 * @param externalId the record's external ID value
 * @param fields an NSDictionary containing field names and values for 
 *               the record, for example, {Name: "salesforce.com", TickerSymbol 
 *               "CRM"}
 * @see http://www.salesforce.com/us/developer/docs/api_rest/Content/resources_sobject_upsert.htm
 */
- (SFRestRequest *)requestForUpsertWithObjectType:(NSString *)objectType
                                  externalIdField:(NSString *)externalIdField
                                       externalId:(nullable NSString *)externalId
                                           fields:(NSDictionary<NSString*, id> *)fields;

/**
 * Returns an `SFRestRequest` which updates field values on a record of the given type.
 * @param objectType object type; for example, "Account"
 * @param objectId the record's object ID
 * @param fields an object containing initial field names and values for 
 *               the record, for example, {Name: "salesforce.com", TickerSymbol 
 *               "CRM"}
 * @see http://www.salesforce.com/us/developer/docs/api_rest/Content/resources_sobject_retrieve.htm
 */
- (SFRestRequest *)requestForUpdateWithObjectType:(NSString *)objectType 
                                         objectId:(NSString *)objectId
                                           fields:(nullable NSDictionary<NSString*, id> *)fields;

/**
 * Same as requestForUpdateWithObjectType:objectId:fields but only executing update
 * if the server record was not modified since ifModifiedSinceDate.
 *
 * @param objectType object type; for example, "Account"
 * @param objectId the record's object ID
 * @param fields an object containing initial field names and values for record
 * @param ifUnmodifiedSinceDate update will only happens if current last modified date of record is
 *                              older than ifUnmodifiedSinceDate
 *                              otherwise a 412 (precondition failed) will be returned
 * @see http://www.salesforce.com/us/developer/docs/api_rest/Content/resources_sobject_retrieve.htm
 */
- (SFRestRequest *)requestForUpdateWithObjectType:(NSString *)objectType
                                         objectId:(NSString *)objectId
                                            fields:(nullable NSDictionary<NSString*, id> *)fields
                            ifUnmodifiedSinceDate:(nullable NSDate *) ifUnmodifiedSinceDate;


/**
 * Returns an `SFRestRequest` which deletes a record of the given type.
 * @param objectType object type; for example, "Account"
 * @param objectId the record's object ID
 * @see http://www.salesforce.com/us/developer/docs/api_rest/Content/resources_sobject_retrieve.htm
 */
- (SFRestRequest *)requestForDeleteWithObjectType:(NSString *)objectType 
                                         objectId:(NSString *)objectId;

/**
 * Returns an `SFRestRequest` which executes the specified SOQL query.
 * @param soql a string containing the query to execute - for example, "SELECT Id, 
 *             Name from Account ORDER BY Name LIMIT 20"
 * @see http://www.salesforce.com/us/developer/docs/api_rest/Content/resources_query.htm
 */
- (SFRestRequest *)requestForQuery:(NSString *)soql;

/**
 * Returns an `SFRestRequest` which executes the specified SOQL query.
 * The result contains the deleted objects.
 * @param soql a string containing the query to execute - for example, "SELECT Id,
 *             Name from Account ORDER BY Name LIMIT 20"
 * @see http://www.salesforce.com/us/developer/docs/api_rest/Content/resources_queryall.htm
 */
- (SFRestRequest *)requestForQueryAll:(NSString *)soql;

/**
 * Returns an `SFRestRequest` which executes the specified SOSL search.
 * @param sosl a string containing the search to execute - for example, "FIND {needle}"
 * @see http://www.salesforce.com/us/developer/docs/api_rest/Content/resources_search.htm
 */
- (SFRestRequest *)requestForSearch:(NSString *)sosl;

/**
 * Returns an `SFRestRequest` which returns an ordered list of objects in the default global search scope of a logged-in user.
 * @see  http://www.salesforce.com/us/developer/docs/api_rest/Content/resources_search_scope_order.htm
 */
- (SFRestRequest *)requestForSearchScopeAndOrder;


/**
 * Returns an `SFRestRequest` which returns search result layout information for the objects in the query string.
 * @param objectList comma-separated list of objects for which
 *               to return values; for example, "Account,Contact".
 * @see  http://www.salesforce.com/us/developer/docs/api_rest/Content/resources_search_layouts.htm
 */
- (SFRestRequest *)requestForSearchResultLayout:(NSString*)objectList;

/**
 * Retursn an `SFRestRequest` which executes a batch of requests.
 * @param requests Array of subrequests to execute.
 * @param haltOnError Controls whether Salesforce should stop processing subrequests if a subrequest fails.
 * @see https://developer.salesforce.com/docs/atlas.en-us.api_rest.meta/api_rest/resources_composite_batch.htm
 */
- (SFRestRequest *) batchRequest:(NSArray<SFRestRequest*>*) requests haltOnError:(BOOL) haltOnError;

/**
 * Retursn an `SFRestRequest` which executes a composite request.
 * @param requests Array of subrequests to execute.
 * @param refIds Array of reference id for the requests (should have the same number of element than requests)
 * @param allOrNone Specifies what to do when an error occurs while processing a subrequest.
 * @see https://developer.salesforce.com/docs/atlas.en-us.api_rest.meta/api_rest/resources_composite_composite.htm
 */
- (SFRestRequest *) compositeRequest:(NSArray<SFRestRequest*>*) requests refIds:(NSArray<NSString*>*)refIds allOrNone:(BOOL) allOrNone;

/**
 * Retursn an `SFRestRequest` which executes a sobject tree request.
 * @param objectType object type; for example, "Account"
 * @param objectTrees Array of sobject trees
 * @see https://developer.salesforce.com/docs/atlas.en-us.api_rest.meta/api_rest/resources_composite_sobject_tree.htm
 */
- (SFRestRequest*) requestForSObjectTree:(NSString*)objectType objectTrees:(NSArray<SFSObjectTree*>*)objectTrees;

///---------------------------------------------------------------------------------------
/// @name Other utility methods
///---------------------------------------------------------------------------------------

+ (BOOL)isStatusCodeSuccess:(NSUInteger)statusCode;

+ (BOOL)isStatusCodeNotFound:(NSUInteger)statusCode;

/**
 * Provides the User-Agent string used by the SDK
 */
+ (NSString *)userAgentString;

/**
 * Returns the User-Agent string used by the SDK, adding the qualifier after the app type.
 @param qualifier Optional sub-type of native or hybrid Mobile SDK app.
 */
+ (NSString *)userAgentString:(NSString*)qualifier;



@end

NS_ASSUME_NONNULL_END
