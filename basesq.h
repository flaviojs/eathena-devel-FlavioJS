// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _SQL_H_
#define _SQL_H_

#include "baseio.h"
#include "basemysql.h"


#if defined(WITH_MYSQL)

#define DEVELOPING_CSQL
#ifdef DEVELOPING_CSQL

///////////////////////////////////////////////////////////////////////////////
NAMESPACE_BEGIN(sq)
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Column type.
/// TODO 
enum ColType {
	// Numeric
	BIT,
	TINYINT,
	SMALLINT,
	MEDIUMINT,
	INT,
	BIGINT,
	FLOATING_POINT,
	FIXED_POINT,

	// Time and Date
	DATE,
	TIME,
	DATETIME,
	TIMESTAMP,

	// Text and Bytes
	TEXT,
	BYTES,
	ENUM,
	BITFIELD // SET
};


///////////////////////////////////////////////////////////////////////////////
/// Reference actions (for OnDelete and OnUpdate).
enum RefAction
{
	ACTION_UNDEFINED,
	ACTION_RESTRICT,
	ACTION_CASCADE,
	ACTION_SETNULL,
	ACTION_NOACTION
};

///////////////////////////////////////////////////////////////////////////////
/// The column only contains unique values (might contain several NULL values).
class Unique
{
public:
	///////////////////////////////////////////////////////////////////////////
	/// Constructors
	Unique(void);

	/// Destructor
	virtual ~Unique(void);
};


///////////////////////////////////////////////////////////////////////////////
/// The column is indexed.
/// TODO multi-column indexes
class Index
{
public:
	///////////////////////////////////////////////////////////////////////////
	/// Constructors
	Index(void);

	/// Destructor
	virtual ~Index(void);
};

///////////////////////////////////////////////////////////////////////////////
/// Reference.
/// TODO multi-column references
class Reference
{
public:
	///////////////////////////////////////////////////////////////////////////
	/// Constructors
	Reference(const basics::string<> &from, const basics::string<> &to, RefAction onDel, RefAction onUp, const basics::string<> &tbl);

	/// Destructor
	virtual ~Reference(void);

	///////////////////////////////////////////////////////////////////////////
	/// Returns the action on delete
	inline const RefAction& onDel(void) const;

	/// Returns the action on update
	inline const RefAction& onUp(void) const;

	/// Returns the action on update
	inline const basics::string<>& table(void) const;

private:
	///////////////////////////////////////////////////////////////////////////
	/// Action on delete.
	const RefAction ref_onDel;

	/// Action on update
	const RefAction ref_onUp;

	/// Target table
	basics::string<> ref_tbl;

	/// From columns
	basics::vector< basics::string<> > ref_from;

	/// To columns
	basics::vector< basics::string<> > ref_to;
};


///////////////////////////////////////////////////////////////////////////////
/// Property.
/// TODO 
template< typename T >
class Property : public basics::defaultcmp
{
public:
	///////////////////////////////////////////////////////////////////////////
	/// Constructors
	Property(void); // required for a vector of properties
	Property(const basics::string<>& name, const basics::string<>& value);

	/// Destructor
	virtual ~Property(void);

	///////////////////////////////////////////////////////////////////////////
	/// Name of the property
	inline const basics::string<>& name(void);

	/// Value of the property
	inline const basics::string<>& value(void);

private:
	///////////////////////////////////////////////////////////////////////////
	/// Name of the property
	basics::string<> prop_name;

	/// Value of the property
	basics::string<> prop_value;
};


///////////////////////////////////////////////////////////////////////////////
/// Identifies a primary column
class AutoIncrements
{
public:
	///////////////////////////////////////////////////////////////////////////
	/// Constructor
	AutoIncrements(const uint32 from);

	/// Destructor
	virtual ~AutoIncrements(void);

	///////////////////////////////////////////////////////////////////////////
	/// Returns the starting index
	inline const uint32 from(void) const;

private:
	///////////////////////////////////////////////////////////////////////////
	/// Starting index
	const uint32 inc_from;
};


///////////////////////////////////////////////////////////////////////////////
/// Identifies a primary column
class Default
{
public:
	///////////////////////////////////////////////////////////////////////////
	/// Constructor
	Default(const uint32 value);
	Default(const basics::string<>& value);

	/// Destructor
	virtual ~Default(void);

	///////////////////////////////////////////////////////////////////////////
	/// Returns the starting index
	inline const basics::string<>& value(void) const;

private:
	///////////////////////////////////////////////////////////////////////////
	/// Start of string
	basics::string<> def_value;
};


///////////////////////////////////////////////////////////////////////////////
/// Identifies a primary column
class Primary
{
public:
	///////////////////////////////////////////////////////////////////////////
	/// Constructor
	Primary(void);

	/// Destructor
	virtual ~Primary(void);
};


///////////////////////////////////////////////////////////////////////////////
/// Column
/// TODO 
class Column : public basics::defaultcmp
{
public:
	///////////////////////////////////////////////////////////////////////////
	/// Constructors
	Column(ColType type, const basics::string<>& name, bool null=true);

	/// Destructor
	virtual ~Column(void);

	///////////////////////////////////////////////////////////////////////////
	/// sets the default value of this column.
	Column& operator<<(Primary& pri);

	/// sets the default value of this column.
	Column& operator<<(Default& value);

	///////////////////////////////////////////////////////////////////////////
	/// Returns the name of the table
	inline const basics::string<>& name() const;

	///////////////////////////////////////////////////////////////////////////
	/// Returns the name of the table
	inline ColType type() const;

	/// Returns if a default value has been assigned
	inline bool hasDefault() const;

	/// Returns the default value
	inline const basics::string<>& defaultsTo() const;

	/// Returns if the column can have NULL values
	inline bool null() const;

private:
	///////////////////////////////////////////////////////////////////////////
	/// type
	ColType col_type;

	/// name
	basics::string<> col_name;

	/// If the column can have NULL values
	bool col_null;

	/// if col_default has been defined
	bool col_hasDefault;

	/// default value
	basics::string<> col_default;
};


///////////////////////////////////////////////////////////////////////////////
/// Int column.
/// TINYINT, SMALLINT, MEDIUMINT, INT, BIGINT
/// T = { int8, uint8, int16, uint16, int32, uint32, int64, uint64 }
template < typename T = uint32 >
class IntColumn : public Column
{
public:
	///////////////////////////////////////////////////////////////////////////
	/// Constructors
	IntColumn(const basics::string<>& name, bool null=true);
	IntColumn(const basics::string<>& name, uint32 digits, bool null=true);

	/// Destructor
	virtual ~IntColumn();
};


///////////////////////////////////////////////////////////////////////////////
/// Text Column
class TextColumn : public Column
{
public:
	///////////////////////////////////////////////////////////////////////////
	/// Constructors
	TextColumn(const basics::string<>& name, bool variable, bool null=true);
	TextColumn(const basics::string<>& name, uint32 size, bool variable, bool null=true);

	/// Destructor
	virtual ~TextColumn();
};


///////////////////////////////////////////////////////////////////////////////
/// Byte Column
class ByteColumn : public Column
{
public:
	///////////////////////////////////////////////////////////////////////////
	/// Constructors
	ByteColumn(const basics::string<>& name, bool null=true);
	ByteColumn(const basics::string<>& name, uint32 size, bool null=true);

	/// Destructor
	virtual ~ByteColumn();
};


///////////////////////////////////////////////////////////////////////////////
/// Enum Column
class EnumColumn : public Column
{
public:
	///////////////////////////////////////////////////////////////////////////
	/// Constructors - add more as needed
	EnumColumn(const basics::string<>& name, const basics::string<>& val1, const basics::string<>& val2, const basics::string<>& val3);

	/// Destructor
	virtual ~EnumColumn();
};


///////////////////////////////////////////////////////////////////////////////
/// Bit Column
class BitColumn : public Column
{
public:
	///////////////////////////////////////////////////////////////////////////
	/// Constructors
	BitColumn(const basics::string<>& name, uint32 size, bool null=true);

	/// Destructor
	virtual ~BitColumn();
};


///////////////////////////////////////////////////////////////////////////////
/// Foreign Reference Column
class RefColumn : public Column, public Reference
{
public:
	///////////////////////////////////////////////////////////////////////////
	/// Constructors
	RefColumn(const basics::string<>& name, RefAction onDel, RefAction onUp, const basics::string<>& ref_tbl);
	RefColumn(const basics::string<>& name, const basics::string<>& ref_col, RefAction onDel, RefAction onUp);
	RefColumn(const basics::string<>& name, const basics::string<>& ref_col, RefAction onDel, RefAction onUp, const basics::string<>& ref_tbl);

	/// Destructor
	virtual ~RefColumn();

	///////////////////////////////////////////////////////////////////////////
	/// Returns if the reference point to the table of this column
	bool sameTable(void) const;

private:
	///////////////////////////////////////////////////////////////////////////
	// If it's a reference to the same table.
	bool ref_sameTable;
};


///////////////////////////////////////////////////////////////////////////////
/// Table.
/// Collection of columns, properties,references and indexes.
class Table : public basics::defaultcmp
{
public:
	Table();
	Table& operator=(const Table& tbl);

public:
	///////////////////////////////////////////////////////////////////////////
	/// Constructor
	Table(const basics::string<>& name);
	Table(const basics::string<>& name, const basics::string<>& engine);

	/// Destructor
	virtual ~Table();

	///////////////////////////////////////////////////////////////////////////
	/// Returns the name of the table.
	inline const basics::string<>& name() const;

	///////////////////////////////////////////////////////////////////////////
	/// Adds a column.
	Table& operator<<(Column& col);

	/// Adds an int column.
	template < typename TT >
	Table& operator<<(IntColumn< TT >& col);

	/// Adds a text column.
	Table& operator<<(TextColumn& col);

	/// Adds a byte column.
	Table& operator<<(ByteColumn& col);

	/// Adds an enum column.
	Table& operator<<(EnumColumn& col);

	/// Adds a bit column.
	Table& operator<<(BitColumn& col);

	/// Adds a reference column.
	Table& operator<<(RefColumn& col);

private:
	///////////////////////////////////////////////////////////////////////////
	/// name
	const basics::string<> tbl_name;

	/// columns
	basics::vector< Column* > tbl_cols;

	/// properties
	basics::vector< Property< Table >* > tbl_props;

	/// references
	basics::vector< Reference* > tbl_refs;

	/// indexes
	basics::vector< Index* > tbl_idxs;
};


///////////////////////////////////////////////////////////////////////////////
/// Table copy.
class CopyTable : Table
{
public:
	///////////////////////////////////////////////////////////////////////////
	/// Constructors
	CopyTable(const basics::string<>& name, const basics::string<>& copy_name);
	CopyTable(const basics::string<>& name, const basics::string<>& copy_name, const basics::string<>& engine);

	/// Destructor
	virtual ~CopyTable();

	///////////////////////////////////////////////////////////////////////////
	/// Returns the name of the copied table
	const basics::string<>& copy_name(void);

private:
	///////////////////////////////////////////////////////////////////////////
	/// name of the table to copy
	basics::string<> tbl_copy_name;

};


///////////////////////////////////////////////////////////////////////////////
/// Database.
/// Collection of tables and properties.
/// TODO table/property access
class Database
{
public:
	///////////////////////////////////////////////////////////////////////////
	/// Constructor.
	Database() {}

	/// Destructor.
	virtual ~Database() {}

	///////////////////////////////////////////////////////////////////////////
	/// Adds a table to the database.
	Database& operator<<(Table& tbl);

	/// Adds a copy of another existing table to the database.
	/// Further changes made to the copy will not propagate to the original.
	Database& operator<<(CopyTable& tbl);

	///////////////////////////////////////////////////////////////////////////
	/// Adds a column to the last table.
	Database& operator<<(Column& col);

	/// Adds an int column to the last table.
	template < typename TT >
	Database& operator<<(IntColumn< TT >& col);

	/// Adds a text column to the last table.
	Database& operator<<(TextColumn& col);

	/// Adds a byte column to the last table.
	Database& operator<<(ByteColumn& col);

	/// Adds an enum column to the last table.
	Database& operator<<(EnumColumn& col);

	/// Adds a bit column to the last table.
	Database& operator<<(BitColumn& col);

	/// Adds a reference column to the last table.
	Database& operator<<(RefColumn& col);

	///////////////////////////////////////////////////////////////////////////
	/// Adds a database property.
	Database& operator<<(Property< Database >& prop);

	/// Adds a property to the last table.
	Database& operator<<(Property< Table >& prop);

	/// Adds a property to the last column of the last table.
	Database& operator<<(Property< Column >& prop);

	///////////////////////////////////////////////////////////////////////////
	/// Sets the last column of the last table as part of the primary key
	/// The column is automatically set to NOT NULL
	Database& operator<<(Primary& p);

	/// Sets the default value of the colunm of the last table
	Database& operator<<(Default& default_);

	/// Sets the last colunm of the last table as incrementable
	Database& operator<<(AutoIncrements& from);

	/// Sets the last column of the last table as unique.
	Database& operator<<(Unique& prop);

	/// Adds an index to the last table
	Database& operator<<(Index& idx);

private:

	/// Returns if 
	bool canAddColumn(const basics::string<>& name) const;

	///////////////////////////////////////////////////////////////////////////
	/// tables
	basics::vector< Table > db_tbls;

	/// properties
	basics::vector< Property< Database > > db_props;
};


///////////////////////////////////////////////////////////////////////////////
NAMESPACE_END(sq)
///////////////////////////////////////////////////////////////////////////////


#endif // DEVELOPING_CSQL

///////////////////////////////////////////////////////////////////////////////
// sql base interface.
// wrapper for the sql handle, table control and parameter storage
class CSQLParameter
{
protected:

	///////////////////////////////////////////////////////////////////////////
	static basics::CMySQL sqlbase;					///< sql handle and connection pool
	///////////////////////////////////////////////////////////////////////////
	static basics::CParam< basics::string<> > mysqldb_id;	///< username
	static basics::CParam< basics::string<> > mysqldb_pw;	///< password
	static basics::CParam< basics::string<> > mysqldb_db;	///< database
	static basics::CParam< basics::string<> > mysqldb_ip;	///< server ip
	static basics::CParam< basics::string<> > mysqldb_cp;	///< server code page
	static basics::CParam< ushort   >         mysqldb_port;	///< server port

	///////////////////////////////////////////////////////////////////////////
	// parameters
	static basics::CParam< basics::string<> > tbl_login_log;
	static basics::CParam< basics::string<> > tbl_char_log;
	static basics::CParam< basics::string<> > tbl_map_log;

	static basics::CParam< basics::string<> > tbl_login_status;
	static basics::CParam< basics::string<> > tbl_char_status;
	static basics::CParam< basics::string<> > tbl_map_status;

	static basics::CParam< basics::string<> > tbl_account;
	
	static basics::CParam< basics::string<> > tbl_char;
	static basics::CParam< basics::string<> > tbl_memo;
	static basics::CParam< basics::string<> > tbl_inventory;
	static basics::CParam< basics::string<> > tbl_cart;
	static basics::CParam< basics::string<> > tbl_skill;
	static basics::CParam< basics::string<> > tbl_friends;

	static basics::CParam< basics::string<> > tbl_mail;

	static basics::CParam< basics::string<> > tbl_login_reg;
	static basics::CParam< basics::string<> > tbl_login_reg2;
	static basics::CParam< basics::string<> > tbl_char_reg;
	static basics::CParam< basics::string<> > tbl_guild_reg;

	static basics::CParam< basics::string<> > tbl_guild;
	static basics::CParam< basics::string<> > tbl_guild_skill;
	static basics::CParam< basics::string<> > tbl_guild_member;
	static basics::CParam< basics::string<> > tbl_guild_position;
	static basics::CParam< basics::string<> > tbl_guild_alliance;
	static basics::CParam< basics::string<> > tbl_guild_expulsion;
	
	static basics::CParam< basics::string<> > tbl_castle;
	static basics::CParam< basics::string<> > tbl_castle_guardian;
	
	static basics::CParam< basics::string<> > tbl_party;
	
	static basics::CParam< basics::string<> > tbl_storage;
	
	static basics::CParam< basics::string<> > tbl_guild_storage;
	
	static basics::CParam< basics::string<> > tbl_pet;
	static basics::CParam< basics::string<> > tbl_homunculus;
	static basics::CParam< basics::string<> > tbl_homunskill;

	static basics::CParam< basics::string<> > tbl_variable;

	static basics::CParam<bool> wipe_sql;
	static basics::CParam< basics::string<> > sql_engine;

	static basics::CParam<bool> log_login;
	static basics::CParam<bool> log_char;
	static basics::CParam<bool> log_map;


	static bool ParamCallback_Database_string(const basics::string<>& name, basics::string<>& newval, const basics::string<>& oldval);
	static bool ParamCallback_Database_ushort(const basics::string<>& name, ushort& newval, const ushort& oldval);
	static bool ParamCallback_Tables(const basics::string<>& name, basics::string<>& newval, const basics::string<>& oldval);


	///////////////////////////////////////////////////////////////////////////
	/// read number of rows from given table
	size_t get_table_size(basics::string<> tbl_name) const
	{
		basics::CMySQLConnection dbcon1(this->sqlbase);
		basics::string<> query;

		query << "SELECT COUNT(*) "
				 "FROM `" << dbcon1.escaped(tbl_name) << "` ";
		
		if( dbcon1.ResultQuery(query) )
		{
			return atol( dbcon1[0] );
		}
		return 0;
	}
	///////////////////////////////////////////////////////////////////////////
	/// constructor.
	/// initialize the database on the first run
	CSQLParameter(const char* configfile)		
	{
		if(configfile) basics::CParamBase::loadFile(configfile);
		static bool first=true;
		if(first)
		{
			this->rebuild();
			first = false;
		}
	}
public:
	///////////////////////////////////////////////////////////////////////////
	/// destructor
	~CSQLParameter()	{}

	///////////////////////////////////////////////////////////////////////////
	// rebuild the tables
	static void rebuild();
};


///////////////////////////////////////////////////////////////////////////////
//
class CAccountDB_sql : public CAccountDBInterface, public CSQLParameter
{
public:
	///////////////////////////////////////////////////////////////////////////
	// construct/destruct
	CAccountDB_sql(const char* configfile=NULL) : CSQLParameter(configfile)
	{
		 this->init(configfile);
	}
	virtual ~CAccountDB_sql()
	{
		close();
	}
protected:
	///////////////////////////////////////////////////////////////////////////
	bool init(const char* configfile);
	bool close();

	bool sql2struct(const basics::string<>& querycondition, CLoginAccount& account);
public:
	///////////////////////////////////////////////////////////////////////////
	// functions for db interface
	virtual size_t size() const;
	virtual CLoginAccount& operator[](size_t i);

	virtual bool existAccount(const char* userid);
	virtual bool searchAccount(const char* userid, CLoginAccount&account);
	virtual bool searchAccount(uint32 accid, CLoginAccount&account);
	virtual bool insertAccount(const char* userid, const char* passwd, unsigned char sex, const char* email, CLoginAccount&account);
	virtual bool removeAccount(uint32 accid);
	virtual bool saveAccount(const CLoginAccount& account);
};

class CCharDB_sql : public CCharDBInterface, public CSQLParameter
{
public:
	CCharDB_sql(const char *dbcfgfile) : CSQLParameter(dbcfgfile)
	{
		init(dbcfgfile);
	}
	virtual ~CCharDB_sql()
	{}
protected:
	///////////////////////////////////////////////////////////////////////////
	// normal function
	bool init(const char* configfile);
	bool close(){ return true; }

public:
	///////////////////////////////////////////////////////////////////////////
	// access interface
	virtual size_t size() const;
	virtual CCharCharacter& operator[](size_t i);

	virtual bool existChar(const char* name);
	virtual bool existChar(uint32 char_id);
	virtual bool searchChar(const char* name, CCharCharacter&data);
	virtual bool searchChar(uint32 char_id, CCharCharacter&data);
	virtual bool insertChar(CCharAccount &account, const char *name, unsigned char str, unsigned char agi, unsigned char vit, unsigned char int_, unsigned char dex, unsigned char luk, unsigned char slot, unsigned char hair_style, unsigned char hair_color, CCharCharacter&data);
	virtual bool removeChar(uint32 charid);
	virtual bool saveChar(const CCharCharacter& data);

	virtual bool searchAccount(uint32 accid, CCharCharAccount& account);
	virtual bool saveAccount(CCharAccount& account);
	virtual bool removeAccount(uint32 accid);


	virtual size_t getMailCount(uint32 cid, uint32 &all, uint32 &unread);
	virtual size_t listMail(uint32 cid, unsigned char box, unsigned char *buffer);
	virtual bool readMail(uint32 cid, uint32 mid, CMail& mail);
	virtual bool deleteMail(uint32 cid, uint32 mid);
	virtual bool sendMail(uint32 senderid, const char* sendername, const char* targetname, const char *head, const char *body, uint32 zeny, const struct item& item, uint32& msgid, uint32& tid);

	virtual void loadfamelist();
};


///////////////////////////////////////////////////////////////////////////////
//
class CGuildDB_sql : public CGuildDBInterface, public CSQLParameter
{
public:
	///////////////////////////////////////////////////////////////////////////
	// construct/destruct
	CGuildDB_sql(const char *dbcfgfile) : CSQLParameter(dbcfgfile)
	{
		this->init(dbcfgfile);
	}
	virtual ~CGuildDB_sql()
	{
		close();
	}

private:
	///////////////////////////////////////////////////////////////////////////
	// normal function
	bool init(const char* configfile);
	bool close()
	{
		return true;
	}

public:
	///////////////////////////////////////////////////////////////////////////
	// access interface
	virtual size_t size() const;
	virtual CGuild& operator[](size_t i);

	virtual size_t castlesize() const;
	virtual CCastle &castle(size_t i);


	virtual bool searchGuild(const char* name, CGuild& guild);
	virtual bool searchGuild(uint32 guildid, CGuild& guild); // TODO: Write
	virtual bool insertGuild(const struct guild_member &member, const char *name, CGuild &g);
	virtual bool removeGuild(uint32 guild_id);
	virtual bool saveGuild(const CGuild& g);

	virtual bool searchCastle(ushort castleid, CCastle& castle);
	virtual bool saveCastle(const CCastle& castle);
	virtual bool removeCastle(ushort castle_id);
	
	virtual bool getCastles(basics::vector<CCastle>& castlevector);
	virtual uint32 has_conflict(uint32 guild_id, uint32 account_id, uint32 char_id);
};

///////////////////////////////////////////////////////////////////////////////
//
class CPartyDB_sql : public CPartyDBInterface, public CSQLParameter
{
public:
	///////////////////////////////////////////////////////////////////////////
	// construct/destruct
	CPartyDB_sql(const char *dbcfgfile) : CSQLParameter(dbcfgfile)
	{
		init(dbcfgfile);
	}

	virtual ~CPartyDB_sql()
	{
		close();
	}
private:
	///////////////////////////////////////////////////////////////////////////
	// normal function
	bool init(const char* configfile);
	bool close()
	{
		return true;
	}
	///////////////////////////////////////////////////////////////////////////
	// access interface
	virtual size_t size() const;
	virtual CParty& operator[](size_t i);

	virtual bool searchParty(const char* name, CParty& p);
	virtual bool searchParty(uint32 pid, CParty& p);
	virtual bool insertParty(uint32 accid, const char* nick, const char* mapname, ushort lv, const char* name, CParty& p);
	virtual bool removeParty(uint32 pid);
	virtual bool saveParty(const CParty& p);
};


///////////////////////////////////////////////////////////////////////////////
// Storage Database Interface
class CPCStorageDB_sql : public CPCStorageDBInterface, public CSQLParameter
{
public:
	///////////////////////////////////////////////////////////////////////////
	// construct/destruct
	CPCStorageDB_sql(const char *dbcfgfile) : CSQLParameter(dbcfgfile)
	{
		init(dbcfgfile);
	}

	~CPCStorageDB_sql()
	{
		close();
	}

	///////////////////////////////////////////////////////////////////////////
	// normal function
	bool init(const char* configfile);
	bool close()
	{
		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	// access interface
	size_t size() const;
	CPCStorage& operator[](size_t i);

	virtual bool searchStorage(uint32 accid, CPCStorage& stor);
	virtual bool removeStorage(uint32 accid);
	virtual bool saveStorage(const CPCStorage& stor);

};

///////////////////////////////////////////////////////////////////////////////
//
class CGuildStorageDB_sql : public CGuildStorageDBInterface, public CSQLParameter
{
public:
	///////////////////////////////////////////////////////////////////////////
	// construct/destruct
	CGuildStorageDB_sql(const char *dbcfgfile) : CSQLParameter(dbcfgfile)
	{
		init(dbcfgfile);
	};
	virtual ~CGuildStorageDB_sql()
	{
		close();
	}

	///////////////////////////////////////////////////////////////////////////
	// access interface
	size_t size() const;
	CGuildStorage& operator[](size_t i);

	virtual bool searchStorage(uint32 gid, CGuildStorage& stor);
	virtual bool removeStorage(uint32 gid);
	virtual bool saveStorage(const CGuildStorage& stor);

private:
	///////////////////////////////////////////////////////////////////////////
	// normal function
	bool init(const char* configfile);
	bool close()
	{
		return true;
	}
};

///////////////////////////////////////////////////////////////////////////////
// Pet Database Interface
class CPetDB_sql : public CPetDBInterface, public CSQLParameter
{
public:
	CPetDB_sql(const char *dbcfgfile) : CSQLParameter(dbcfgfile)
	{
		init(dbcfgfile);
	}
	virtual ~CPetDB_sql()
	{
		close();
	}
protected:
	bool init(const char *dbcfgfile);
	bool close()
	{
		return true;
	}
public:
	virtual size_t size() const;
	virtual CPet& operator[](size_t i);

	virtual bool searchPet(uint32 pid, CPet& pet);
	virtual bool insertPet(uint32 accid, uint32 cid, short pet_class, short pet_lv, short pet_egg_id, ushort pet_equip, short intimate, short hungry, char renameflag, char incuvat, char *pet_name, CPet& pet);
	virtual bool removePet(uint32 pid);
	virtual bool savePet(const CPet& pet);
};


///////////////////////////////////////////////////////////////////////////////
// Homunculi Database Interface
class CHomunculusDB_sql : public CHomunculusDBInterface, public CSQLParameter
{
public:
	CHomunculusDB_sql(const char *dbcfgfile) : CSQLParameter(dbcfgfile)
	{
		init(dbcfgfile);
	}
	virtual ~CHomunculusDB_sql()
	{
		close();
	}
protected:
	bool init(const char *dbcfgfile);
	bool close()
	{
		return true;
	}
public:
	virtual size_t size() const;
	virtual CHomunculus& operator[](size_t i);

	virtual bool searchHomunculus(uint32 hid, CHomunculus& hom);
	virtual bool insertHomunculus(CHomunculus& hom);
	virtual bool removeHomunculus(uint32 hid);
	virtual bool saveHomunculus(const CHomunculus& hom);
};



///////////////////////////////////////////////////////////////////////////////
// Variable Database Interface
// testcase, possibly seperate into different implementations
class CVarDB_sql : public CVarDBInterface, public CSQLParameter
{
public:
	CVarDB_sql(const char *dbcfgfile) : CSQLParameter(dbcfgfile)
	{
		init(dbcfgfile);
	}
	virtual ~CVarDB_sql()
	{
		close();
	}
protected:
	bool init(const char *dbcfgfile);
	bool close()
	{
		return true;
	}
public:
	///////////////////////////////////////////////////////////////////////////
	// access interface
	virtual size_t size() const;
	virtual CVar& operator[](size_t i);


	virtual bool searchVar(const char* name, CVar& var);
	virtual bool insertVar(const char* name, const char* value);
	virtual bool removeVar(const char* name);
	virtual bool saveVar(const CVar& var);
};


#endif// defined(WITH_MYSQL)

#endif //_SQL_H_
