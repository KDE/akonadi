Resource.setType( "akonadi_knut_resource" );

// read test
Resource.setPathOption( "DataFile", "knutdemo.xml" );
Resource.setOption( "FileWatchingEnabled", false );
Resource.create();

XmlOperations.setXmlFile( "knutdemo.xml" );
XmlOperations.setRootCollections( Resource.identifier() );
XmlOperations.assertEqual();

Resource.destroy();

// empty resource
Resource.setPathOption( "DataFile", "newfile.xml" );
Resource.create();

XmlOperations.setXmlFile( "knut-empty.xml" );
XmlOperations.setRootCollections( Resource.identifier() );
XmlOperations.assertEqual();

// folder creation
CollectionTest.setParent( "Knut test data" );
CollectionTest.addContentType( "message/rfc822" );
CollectionTest.setName( "test folder" );
CollectionTest.create();
//Resource.recreate();

// item creation
ItemTest.setParentCollection( "Knut test data/test folder" );
ItemTest.setMimeType( "message/rfc822" );
ItemTest.setPayloadFromFile( "testmail.mbox" );
ItemTest.create();

Resource.recreate();

XmlOperations.setXmlFile( "knut-step1.xml" );
XmlOperations.setRootCollections( Resource.identifier() );
XmlOperations.setCollectionKey( "None" );
XmlOperations.ignoreCollectionField( "RemoteId" );
XmlOperations.setItemKey( "None" );
XmlOperations.ignoreItemField( "RemoteId" );
XmlOperations.assertEqual();

// folder modification
CollectionTest.setCollection( "Knut test data/test folder" );
CollectionTest.setName( "changed folder" );
CollectionTest.update();

Resource.recreate();

XmlOperations.setXmlFile( "knut-step2.xml" );
XmlOperations.setRootCollections( Resource.identifier() );
XmlOperations.assertEqual();

// folder deletion
CollectionTest.setCollection( "Knut test data/changed folder" );
CollectionTest.remove();

Resource.recreate();

XmlOperations.setXmlFile( "knut-empty.xml" );
XmlOperations.setRootCollections( Resource.identifier() );
XmlOperations.assertEqual();

