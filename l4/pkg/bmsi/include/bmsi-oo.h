// Object Oriented C header file for package opentc.wp4.ManagementInterface
/* Note: Unless stated otherwise, when a pointer is passed by reference to a method, the pointer is allocated by the method, and should be free by the caller when it doesn't require it anymore */
// Basic types definition
#include <stdint.h>
typedef enum {DomainStatusCreated, DomainStatusAllocated, DomainStatusRunning, DomainStatusPaused, DomainStatusTerminated} DomainStatus;

typedef uint8_t boolean;

typedef int32_t int32;

typedef int64_t int64;

typedef uint8_t byte;

typedef void * BuilderInitializer;

typedef void * DomainInitializer;

typedef void * PDDescriptor;

// Currently all the interfaces pointers are defined as an abstract data type. This might be replaced by Hypervisor specific definitions
typedef void * BMSI;
typedef void * PDBuilder;
typedef void * ProtectionDomain;
typedef void * IntegrityInterface;
/* ********************************************************************
 * Definitions of structures for package opentc.wp4.ManagementInterface
 * ********************************************************************/
typedef struct {
	/** Maximal size of memory available to the Protection Domain expressed in Bytes.The actual set value might be rounded by the BMSI Implementation depending on the underlying hypervisor. **/
	long MemoryMax;
	/** Amount of memory currently allocated to the Protection Domain. This will be rounded up by the BMSI implementation. The granularity depends on the hypervisor used. **/
	long MemoryCurrent;
	int32 Priority;
	byte NbrCpu;

} Resources;
/* ********************************************************************
 * Definitions of functions for package opentc.wp4.ManagementInterface
 * ********************************************************************/
/* ******************************************
 * Interface: BMSI
 * ******************************************/
/** Returns the version of the API implemented by this interface. **/

int BMSI_getVersion(int* version);

/** Gets a reference to the builder to use to create a new Protection Domain. **/

int BMSI_getBuilder(BuilderInitializer Params, PDBuilder* Builder);

/** Gives a Protection Domain a reference to its integrity interface. **/

int BMSI_getIntegrityInterface(IntegrityInterface* IntInterface);

/* ******************************************
 * Interface: PDBuilder
 * ******************************************/
/** Creates a new protection domain. **/

int PDBuilder_createPD(PDBuilder Self, DomainInitializer Params, ProtectionDomain* PDomain);

/** Tries to get a reference for an existing Protection Domain based on its local (hypervisor specific) ID. **/

int PDBuilder_findPD(PDBuilder Self, int64 LocalId, ProtectionDomain* PDomain);

int PDBuilder_free(PDBuilder Self);

/* ******************************************
 * Interface: ProtectionDomain
 * ******************************************/
/** Specifies the amount of resources that should be allocated to the protection domain. This can only be specified on a protection domain that has not be built yet, and that is not yet running. **/

int ProtectionDomain_setResources(ProtectionDomain Self, Resources Res);

/** Get the current resources allocated and used by a domain. The caller must have enough permissions to call this function. **/

int ProtectionDomain_getResources(ProtectionDomain Self, Resources* Res);

int ProtectionDomain_build(ProtectionDomain Self, PDDescriptor Image);

/** Gets the current status of the Protection Domain. **/

int ProtectionDomain_getStatus(ProtectionDomain Self, DomainStatus* Status);

/** Request the Protection Domain to switch to the Running state (DomainStatusRunning). **/

int ProtectionDomain_run(ProtectionDomain Self);

/** Request the Protection Domain to switch to the Paused status (DomainStatusPaused) **/

int ProtectionDomain_pause(ProtectionDomain Self);

/** Destroys the Protection Domain controlled by this interface and free all resources the Protection Domain was using. Depending on the hypervisor used, this function works recursively and will free resources allocated by child Protection Domains (if any). The Protection Domain must be in the DomainStatusRunning or DomainStatusPaused state for this operation to be allowed. The Protection Domain will be in the DomainStatusTerminated state following the destroy operation. **/

int ProtectionDomain_destroy(ProtectionDomain Self);

/** Allow a connection (or disallow a previously allowed connection) to take place between the Protection Domain represented by the PDcontroller instance and another Protection Domain on the same platform referred to by Dest parameter. The connection will be used by the controlled Protection Domain to require services from the Protection Domain identified by Dest. By default connections are not allowed unless specifically allowed by this method. Protection Domains will then use the usual implementation specific interface provided by the Hypervisor to open connections to other Protection Domains. This operation will succeed only if the connections were specifically allowed using this method. Disalowing an already opened connection does not close the connection.This method enables connection to be established FROM the Protection Domain referenced by this interface TO the Protection Domain referenced by Dest. So, in order to establish a bi-directional channel, the same method must be called on the Dest Protection Domain to allow connection with this one. Also, since the underlying hypervisor might not provide half-duplex communication channel, a one way connection might not be possible. In such a case, no connection will be allowed until the allowConnection method is also used to allow connection from Dest Protection Domain to this Protection Domain. **/

int ProtectionDomain_allowConnection(ProtectionDomain Self, ProtectionDomain Dest, boolean Allow);

/** Get the local id assigned by the hypervisor to this Protection Domain. **/

int ProtectionDomain_getLocalId(ProtectionDomain Self, int64* LocalId);

int ProtectionDomain_free(ProtectionDomain Self);

/* ******************************************
 * Interface: IntegrityInterface
 * ******************************************/
/** Extend the Protection Domain Configuration Register (DCR) with the provided hash. **/

int IntegrityInterface_extend(IntegrityInterface Self, int Hash_size, byte *  Hash);

/** Request protection of a secret by the BMSI Service. This secret will then be accessible through the unseal method. **/

int IntegrityInterface_seal(IntegrityInterface Self, int Secret_size, byte *  Secret, int * Blob_size, byte*  *  Blob);

/** Request access to a secret protected by the BMSI Service **/

int IntegrityInterface_unseal(IntegrityInterface Self, int Blob_size, byte *  Blob, int * Data_size, byte*  *  Data);

/** Requires a cryptographic proof of the integrity of the Protection Domain and of the underlying software (typically the Trusted Virtualization Layer and the Basic Management and Security Service **/

int IntegrityInterface_quote(IntegrityInterface Self, int ExternalData_size, byte *  ExternalData, int * Proof_size, byte ** Proof);

int IntegrityInterface_free(IntegrityInterface Self);

