// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _SQL_H_
#define _SQL_H_

#include "baseexceptions.h"
#include "baseio.h"
#include "basemysql.h"


#if defined(WITH_MYSQL)

#define DEVELOPING_CSQL
#ifdef DEVELOPING_CSQL

template <typename T>
struct typemarker
{
	typedef T value_type;
};

template <typename CON> class CSQLReference;
template <typename CON> class CSQLColumn;
template <typename CON> class CSQLTable;
template <typename CON> class CSQLDatabase;

///////////////////////////////////////////////////////////////////////////////
/// Enumeration of SQL column types
enum CSQLColumnType {
	// Numeric
	CSQL_BIT,
	CSQL_TINYINT,
	CSQL_SMALLINT,
	CSQL_MEDIUMINT,
	CSQL_INT,
	CSQL_BIGINT,
	CSQL_FLOATING_POINT,
	CSQL_FIXED_POINT,

	// Time and Date
	CSQL_DATE,
	CSQL_TIME,
	CSQL_DATETIME,
	CSQL_TIMESTAMP,

	// Text and Bytes
	CSQL_TEXT,
	CSQL_BYTES,
	CSQL_ENUM,
	CSQL_BITFIELD // SET
};

/// Enumeration of reference actions
enum CSQLReferenceAction
{
	CSQL_ACTIONUNDEFINED,
	CSQL_RESTRICT,
	CSQL_CASCADE,
	CSQL_SETNULL,
	CSQL_NOACTION,
};

///////////////////////////////////////////////////////////////////////////////
/// Class to know if column type and the signess of integer columns.
/// TYPE={int8,uint8,int16,uint16,int32,uint32,int64,uint64}
/// (can't do partial member function specialization /pif)
template <typename TYPE>
class CSQLIntCol
{
public:
	/// Returns the type of the integer column
	inline static CSQLColumnType type();
	/// Returns the type of the integer column according to the number of digits
	inline static CSQLColumnType type(uint32 digits)
	{
		return ( digits < 3 ? CSQL_TINYINT :
				digits < 5 ? CSQL_SMALLINT :
				digits < 7 ? CSQL_MEDIUMINT :
				digits < 10 ? CSQL_INT :
				CSQL_BIGINT );
	}
	/// Returns if the integer column is unsigned
	inline static bool isUnsigned();
};

///////////////////////////////////////////////////////////////////////////////
/// SQL foreign key.
template <typename CON>
class CSQLReference
{
	friend class CSQLReference<CON>;
	friend class CSQLColumn<CON>;
	friend class CSQLTable<CON>;
	friend class CSQLDatabase<CON>;

	/// Parent table.
	CSQLTable<CON> &tbl;

	/// Target table.
	CSQLTable<CON> &target;

	/// Map of column references.
	basics::map< const CSQLColumn<CON> *, CSQLColumn<CON> * > refs;

	/// temporary from column.
	CSQLColumn<CON> *from_;

	/// Action on delete
	CSQLReferenceAction onDel;

	/// Action on update
	CSQLReferenceAction onUp;

	///////////////////////////////////////////////////////////////////////////
	/// Constructor.
	CSQLReference(CSQLTable<CON> &tbl, CSQLTable<CON> &target)
		: tbl(tbl)
		, target(target)
		, from_(NULL)
	{
		onDel = CSQL_ACTIONUNDEFINED;
		onUp = CSQL_ACTIONUNDEFINED;
	}

public:

	///////////////////////////////////////////////////////////////////////////
	/// Returns the parent table.
	CSQLTable<CON> &table()	{ return *tbl; }

	/// Returns the parent database.
	CSQLDatabase<CON> &db()	{ return *tbl->db_; }

	///////////////////////////////////////////////////////////////////////////
	/// Sets the from column.
	CSQLReference<CON> &from(CSQLColumn<CON> &col)
	{
		if( &col.tbl != &tbl )
			printf("CSQLReference: From column '%s.%s' does not belong to table '%s'.",
				col.tbl.name, col.name, tbl.name);// Warning
		else if( from_ )
			printf("CSQLReference: Expecting a to column for '%s.%s', discarting from column '%s.%s'.",
					tbl.name, from_->name, col.tbl.name, col.name);// Warning
		else if( refs.exists(&col) )
			printf("CSQLReference: From column '%s.%s' is already referenced, ignoring.");// Warning
		else
			from_ = &col;
		return *this;
	}
	/// Sets the from column.
	CSQLReference<CON> &from(const basics::string<> &col)
	{
		if( tbl->cols.exists(col) )
			return from(*tbl->cols[col]);
		printf("CSQLReference: From column '%s.%s' does not exist, ignoring.",
				tbl->name, col);// Warning
		return *this;
	}

	///////////////////////////////////////////////////////////////////////////
	/// Sets the to column.
	CSQLReference<CON> &to(CSQLColumn<CON> &col)
	{
		if( &col.tbl != &target )
			printf("CSQLReference: To column '%s.%s' does not belong to table '%s'.",
				col.tbl.name, col.name, target.name);// Warning
		else if( !from_ )
			printf("CSQLReference: From column undefined for table '%s', discarting to column '%s.%s'.",
					tbl.name, col.tbl.name, col.name);// Warning
		else
		{
			refs.insert(from_,&col);
			from_ = NULL;
		}
		return *this;
	}
	/// Sets the to column.
	CSQLReference<CON> &to(const basics::string<> &col)
	{
		if( target->cols.exists(col) )
			return to((*target)[col]);
		printf("CSQLReference: To column '%s.%s' does not exist, ignoring (table '%s').",
				target->name, col, tbl->name);// Warning
		return *this;
	}

	///////////////////////////////////////////////////////////////////////////
	/// Sets a reference of columns of the same name.
	CSQLReference<CON> &cols(const basics::string<> &col)	{ return from(col).to(col);	}

	///////////////////////////////////////////////////////////////////////////
	/// Sets the on delete action
	CSQLReference<CON> &onDelete(CSQLReferenceAction action)	{ onDel = action; return *this; }

	/// Sets the on update action
	CSQLReference<CON> &onUpdate(CSQLReferenceAction action)	{ onUp = action; return *this; }

};

///////////////////////////////////////////////////////////////////////////////
/// SQL column definition.
//## maybe break up into objects by the column base type
template <typename CON>
class CSQLColumn
{
	friend class CSQLReference<CON>;
	friend class CSQLColumn<CON>;
	friend class CSQLTable<CON>;
	friend class CSQLDatabase<CON>;

	/// Parent table.
	CSQLTable<CON> &tbl;

	/// Column type.
	CSQLColumnType type_;

	/// Column name.
	basics::string<> name;

	/// Default value of the column
	basics::string<> default_;
	/// The status of the default_ variable.
	enum {
		DEF_UNDEFINED,
		DEF_DEFINED,
		DEF_NULL
	} defaultStatus;

	/// If this column is part of the primary key.
	bool primary;

	/// If this column is nullable.
	bool nullable;

	/// If this column is indexed.
	bool indexed;

	/// If this column only contains unique values.
	/// The behaviour for NULL values depends on the table engine being used.
	bool unique;

	/// Column properties
	union {

		/// Text and byte column properties
		struct {
			bool variable; // If the column has variable size
		} textByte;

		/// Numeric column properties
		struct {
			bool unsigned_; // If the column is unsigned.
		} numeric;

		/// Enum and bitfield columns
		basics::vector< basics::string<> > *values;

	} prop;

	/// Column reference. (aka FOREIGN KEY)
	CSQLColumn<CON> *refTo;

	/// Columns using this column as reference.
	basics::vector< CSQLColumn<CON> * > refFrom;

	///
	uint32 val1;

	///
	uint32 val2;

	///////////////////////////////////////////////////////////////////////////
	/// Constructors.
	/// the meaning of val1 and val2 depend on the type of column
	CSQLColumn(CSQLTable<CON> &tbl, CSQLColumnType type, const basics::string<> &name,
			uint32 val1=UINT32_MAX, uint32 val2=UINT32_MAX)
		: tbl(tbl), type_(type), name(name)
	{
		init(val1,val2);
	}

	/// Creates a copy of another column to be referenced
	/// Assumes the column is valid
	CSQLColumn(CSQLTable<CON> &tbl, CSQLColumn<CON> &col, const basics::string<> &name)
		: tbl(tbl), type_(col.type_), name(name)
	{
		copy(col);
	}

	/// Destructor.
	~CSQLColumn()
	{
		if( containsValues() )
			delete prop.values;
	}

	///////////////////////////////////////////////////////////////////////////
	/// Load the column defaults.
	void init(uint32 val1,uint32 val2)
	{
		defaultStatus = DEF_UNDEFINED;
		this->val1 = val1;
		this->val2 = val2;
		primary = false;
		nullable = true;
		indexed = false;
		unique = false;
		refTo = NULL;
		if( isTextByte() )
			prop.textByte.variable = false;
		else if( isNumeric() )
			prop.numeric.unsigned_ = false;
		else if( containsValues() )
			prop.values = new basics::vector< basics::string<> >();
	}

	/// Copy the data of the column (for column references)
	void copy(CSQLColumn<CON> &col)
	{
		defaultStatus = col.defaultStatus;
		default_ = col.default_;
		val1 = col.val1;
		val2 = col.val2;
		primary = false;
		nullable = col.nullable;
		indexed = true;
		unique = col.unique;
		refTo = NULL;
		if( isTextByte() )
			prop.textByte.variable = col.prop.textByte.variable;
		else if( isNumeric() )
			prop.numeric.unsigned_ = col.prop.numeric.unsigned_;
		else if( containsValues() )
			prop.values = new basics::vector< basics::string<> >(*col.prop.values);
		default_ = col.default_;
	}

	bool containsValues()	{ return type_ == CSQL_ENUM || type_ == CSQL_BITFIELD; }
	bool isTextByte()		{ return type_ == CSQL_TEXT || type_ == CSQL_BYTES; }
	bool isNumeric()		{ return type_ >= CSQL_BIT && type_ <= CSQL_FIXED_POINT; }

public:

	///////////////////////////////////////////////////////////////////////////
	// All column types

	/// Sets the default value.
	/// The value is converted to a string.
	template <typename TYPE>
	CSQLColumn<CON> &defaultsTo(TYPE val)	{ defaultStatus = DEF_DEFINED; default_.clear(); default_ << val; return *this; }
	CSQLColumn<CON> &defaultsToNull()		{ defaultStatus = DEF_NULL; default_.clear(); return *this; }

	/// Sets this column as not null.
	CSQLColumn<CON> &isNotNull()	{ nullable = false; return *this; }

	/// Includes this column in the primary key
	/// isNotNull() is invoked automatically
	CSQLColumn<CON> &isPrimary()	{ return tbl.setPrimary(*this); }

	/// Sets this column as indexed.
	CSQLColumn<CON> &isIndexed()	{ indexed = true; return *this; }

	/// Sets this column as unique.
	CSQLColumn<CON> &isUnique()	{ unique = true; return *this; }

	/// Create a reference from this column to another
	/// The column is automatically indexed
	CSQLReference<CON> &refersTo(CSQLColumn<CON> &col)
	{
		return tbl.references(col.tbl).from(*this).to(col);
	}

	///////////////////////////////////////////////////////////////////////////
	// Text and bytes column types

	/// Sets the size of this text or bytes column as variable.
	CSQLColumn<CON> &isVariable()
	{
		if( isTextByte() )
			prop.textByte.variable = true;
		else
			printf("CSQLColumn: Column '%s.%s' can't be variable.", tbl.name, name);// Warning
		return *this;
	}

	/// Sets this column as auto-incrementable
	CSQLColumn<CON> &autoIncrements()
	{
		return tbl.setAutoInc(*this);
	}
	template <typename TYPE>
	CSQLColumn<CON> &autoIncrements(TYPE val)
	{
		return tbl.setAutoInc<TYPE>(*this, val);
	}

	///////////////////////////////////////////////////////////////////////////
	// Enum/bitfield columns

	/// Adds a value to this enum/bitfield column.
	/// The value is converted to a string.
	template <typename TYPE>
	CSQLColumn<CON> &has(TYPE val)
	{
		if( containsValues() )
		{
			basics::string<> str;
			basics::vector< basics::string<> >::iterator it(*prop.values);

			// check for empty string (always enum value 0)
			str << val;
			if( type_ == CSQL_ENUM && str.length() == 0 )
				return *this;

			// check for duplicates
			for( ; it ; ++it )
			{
				if( *it == str )
				{
					printf("CSQLColumn: Column '%s.%s' already contains \"%s\".", tbl.name, name, str);// Warning
					return *this;
				}
			}

			// check for space (bitfield can have 64 values, enum 65535)
			if( prop.values->size() >= 64 && (type_ == CSQL_BITFIELD ||
					(prop.values->size() == 65535 && type_ == CSQL_ENUM)) )
			{
				printf("CSQLColumn: Column '%s.%s' is full, ignoring \"%s\".", tbl.name, name, str);// Warning
				return *this;
			}
			prop.values->append(str);
		}
		else
			printf("CSQLColumn: Column '%s.%s' can't ´contain values.", tbl.name, name);// Warning
		return *this;
	}

	///////////////////////////////////////////////////////////////////////////
	/// Returns the parent table.
	CSQLTable<CON> &table()	{ return tbl; };

	/// Returns the parent database.
	CSQLDatabase<CON> &db()	{ return tbl.db_; };

};

///////////////////////////////////////////////////////////////////////////////
/// SQL table definition.
/// Needs to have at least 1 column
template <typename CON>
class CSQLTable
{
	friend class CSQLReference<CON>;
	friend class CSQLColumn<CON>;
	friend class CSQLTable<CON>;
	friend class CSQLDatabase<CON>;

	/// Parent database.
	CSQLDatabase<CON> &db_;

	/// Name of the table.
	const basics::string<> name;

	/// Map of table columns.
	basics::map<basics::string<>, CSQLColumn<CON> * > cols;

	/// Vector of primary key columns.
	basics::vector< CSQLColumn<CON> * > primary;

	/// Vector of table references.
	basics::vector< CSQLReference<CON> * > refs;

	/// Table engine.
	basics::string<> engine;

	/// Auto-incrementable column. Only one per table.
	struct _autoInc {
		CSQLColumn<CON> *col;
		basics::string<> start;
	} autoInc;

	///////////////////////////////////////////////////////////////////////////
	/// Constructor.
	/// @param db Database object
	/// @param name Table name
	CSQLTable<CON>(CSQLDatabase<CON> &db, const basics::string<> &name)
		: db_(db)
		, name(name)
	{
		autoInc.col = NULL;
	}
	/// Destructor
	/// Deletes the columns.
	~CSQLTable<CON>()
	{
		// delete columns
		typename basics::map<basics::string<>, CSQLColumn<CON> * >::iterator it(cols);
		for( ; it ; ++it )
			delete it->data;
		cols.clear();
	}

	///////////////////////////////////////////////////////////////////////////
	/// Inserts a column in this table and returns the column.
	CSQLColumn<CON> &insert(const basics::string<> &name, CSQLColumn<CON> *col)
	{
		if( cols.exists(name) )
		{
			printf("CSQLTable: Column '%s.%s' already exists, using previous definition.");// Warning
			delete col;
			return *cols[name];
		}
		cols.insert(name,col);
		return *col;
	}

	/// Adds the column to the primary key.
	/// Sets column not null.
	CSQLColumn<CON> &setPrimary(CSQLColumn<CON> &col)
	{
		if( !col.primary )
		{
			col.primary = true;
			primary.append(&col);
			return col.isNotNull(); // Primary keys can't be NULL
		}
		return col;
	}

	/// Sets the auto-incrementable column.
	CSQLColumn<CON> &setAutoInc(CSQLColumn<CON> &col)
	{
		if( autoInc.col )
			printf("CSQLTable: Column '%s.%s' is already auto-incrementable, ignoring '%s.%s'.",
					name, autoInc.col->name, name, col.name);// Warning
		else
			autoInc.col = &col;
		return col;
	}
	/// Sets the auto-incrementable column.
	template <typename TYPE>
	CSQLColumn<CON> &setAutoInc(CSQLColumn<CON> &col, TYPE val)
	{
		if( autoInc.col )
			printf("CSQLTable: Column '%s.%s' is already auto-incrementable, ignoring '%s.%s'.",
					name, autoInc.col->name, name, col.name);// Warning
		else
		{
			autoInc.col = &col;
			autoIncrementsFrom(val);
		}
		return col;
	}
public:

	///////////////////////////////////////////////////////////////////////////
	/// Returns the parent database.
	CSQLDatabase<CON> &db()	{ return db; }

	/// Sets the table engine (InnoDB/MyISAM/...).
	CSQLTable<CON> &usingEngine(const basics::string<> &engine)	{ this->engine = engine; return *this; }

	/// Sets the starting value of the auto-incrementing column.
	template <typename TYPE>
	CSQLTable<CON> &autoIncrementsFrom(TYPE val)
	{
		if( autoInc.col )
		{
			autoInc.start.clear();
			autoInc.start << val;
		}
		else
			printf("CSQLTable: Auto-incrementable column is undefined in table '%s', ignoring value.", name);// Warning
		return *this;
	}

	///////////////////////////////////////////////////////////////////////////
	/// Create a new table reference. (aka, foreign key)
	CSQLReference<CON> &references(CSQLTable<CON> &target)
	{
		CSQLReference<CON> *ref = new CSQLReference<CON>(*this,target);
		refs.insert(ref);
		return *ref;
	}
	CSQLReference<CON> &references()	{ return references(*this); }

	///////////////////////////////////////////////////////////////////////////
	// Numeric types
	CSQLColumn<CON> &bitCol(const basics::string<> &name, uint32 bits)		{ return insert(name,new CSQLColumn<CON>(*this,CSQL_BIT,name,bits)); }
	template <typename TYPE>
	CSQLColumn<CON> &intCol(const basics::string<> &name, const typemarker<TYPE>&)
	{
		CSQLColumn<CON> &col = insert(name,new CSQLColumn<CON>(*this,CSQLIntCol<TYPE>::type(),name));
		col.prop.numeric.unsigned_ = CSQLIntCol<TYPE>::isUnsigned();
		return col;
	}
	template <typename TYPE>
	CSQLColumn<CON> &intCol(const basics::string<> &name, uint32 digits, const typemarker<TYPE>&)
	{
		CSQLColumn<CON> &col = insert(name,new CSQLColumn<CON>(*this,CSQLIntCol<TYPE>::type(digits),name,digits));
		col.prop.numeric.unsigned_ = CSQLIntCol<TYPE>::isUnsigned();
		return col;
	}

	// Floating-point types
	template <typename X>
	CSQLColumn<CON> &floatingPointCol(const basics::string<> &name, const typemarker<X>&); // [0,24] bits -> FLOAT , [25,53] bits -> DOUBLE
	CSQLColumn<CON> &floatingPointCol(const basics::string<> &name, uint32 bits)					{ return insert(name,new CSQLColumn<CON>(*this,CSQL_FLOATING_POINT,name,bits)); }
	CSQLColumn<CON> &floatingPointCol(const basics::string<> &name, uint32 digits, uint32 decimals)	{ return insert(name,new CSQLColumn<CON>(*this,CSQL_FLOATING_POINT,name,digits,decimals)); }

	// Fixed-point types
	CSQLColumn<CON> &fixedPointCol(const basics::string<> &name)									{ return insert(name,new CSQLColumn<CON>(*this,CSQL_FIXED_POINT,name)); }
	CSQLColumn<CON> &fixedPointCol(const basics::string<> &name, uint32 digits)						{ return insert(name,new CSQLColumn<CON>(*this,CSQL_FIXED_POINT,name,digits)); }
	CSQLColumn<CON> &fixedPointCol(const basics::string<> &name, uint32 digits, uint32 decimals)	{ return insert(name,new CSQLColumn<CON>(*this,CSQL_FIXED_POINT,name,digits,decimals)); }

	///////////////////////////////////////////////////////////////////////////
	// Date and Time types
	CSQLColumn<CON>      &dateCol(const basics::string<> &name)	{ return insert(name,new CSQLColumn<CON>(*this,CSQL_DATE,name)); }
	CSQLColumn<CON>      &timeCol(const basics::string<> &name)	{ return insert(name,new CSQLColumn<CON>(*this,CSQL_TIME,name)); }
	CSQLColumn<CON>  &datetimeCol(const basics::string<> &name)	{ return insert(name,new CSQLColumn<CON>(*this,CSQL_DATETIME,name)); }
	CSQLColumn<CON> &timestampCol(const basics::string<> &name)	{ return insert(name,new CSQLColumn<CON>(*this,CSQL_TIMESTAMP,name)); }

	///////////////////////////////////////////////////////////////////////////
	// Text and Byte types
	CSQLColumn<CON>     &textCol(const basics::string<> &name)					{ return insert(name,new CSQLColumn<CON>(*this,CSQL_TEXT,name)); }
	CSQLColumn<CON>     &textCol(const basics::string<> &name, uint32 length)	{ return insert(name,new CSQLColumn<CON>(*this,CSQL_TEXT,name,length)); }
	CSQLColumn<CON>     &byteCol(const basics::string<> &name)					{ return insert(name,new CSQLColumn<CON>(*this,CSQL_BYTES,name)); }
	CSQLColumn<CON>     &byteCol(const basics::string<> &name, uint32 length)	{ return insert(name,new CSQLColumn<CON>(*this,CSQL_BYTES,name,length)); }
	CSQLColumn<CON>     &enumCol(const basics::string<> &name)					{ return insert(name,new CSQLColumn<CON>(*this,CSQL_ENUM,name)); }
	CSQLColumn<CON> &bitfieldCol(const basics::string<> &name)					{ return insert(name,new CSQLColumn<CON>(*this,CSQL_BITFIELD,name)); }

	///////////////////////////////////////////////////////////////////////////
	/// Creates a copy of another column, automatically creating a reference to it
	/// The column is automatically indexed
	CSQLColumn<CON> &referenceCol(const basics::string<> &alias, CSQLColumn<CON> &col, CSQLReferenceAction onDel=CSQL_ACTIONUNDEFINED, CSQLReferenceAction onUp=CSQL_ACTIONUNDEFINED)
		throw(basics::exception)
	{
		CSQLColumn<CON> *newCol;
		if( &db_ != &col.tbl.db_ )
		{// wrong database
			basics::string<> err;
			err << "CSQLTable: Column '" << col.tbl.name << "." << col.name
				<< "' doesn't belong to the same database, aborting reference from '"
				<< name << "." << alias << "'.";
			throw basics::exception(err);
		}
		else if( cols.exists(alias) )
		{// alrady exists
			printf("CSQLTable: Column '%s.%s' already exists, creating reference to '%s.%s'",
					name, alias, col.tbl.name, col.name);// Warning
			newCol = cols[alias];
		}
		else
			newCol = new CSQLColumn<CON>(*this,col,alias);
		newCol->refersTo(col).onDelete(onDel).onUpdate(onUp);
		return *newCol;
	}
	CSQLColumn<CON> &referenceCol(CSQLColumn<CON> &col, CSQLReferenceAction onDel=CSQL_ACTIONUNDEFINED, CSQLReferenceAction onUp=CSQL_ACTIONUNDEFINED)
		throw(basics::exception)
	{ return referenceCol(col.name,col,onDel,onUp); }

	///////////////////////////////////////////////////////////////////////////
	/// Retrieves a child column.
	/// @param name Column name
	CSQLColumn<CON> &operator[](const basics::string<> &name) throw(basics::exception)
	{
		if( cols.exists(name) )
			return *cols[name];
		else
		{
			basics::string<> err;
			err << "CSQLTable: Column '" << name << "' not found.";
			throw basics::exception(err);
		}
	}

};

///////////////////////////////////////////////////////////////////////////////
/// SQL database definition.
/// Contains tables.
template <typename CON>
class CSQLDatabase
{
	friend class CSQLReference<CON>;
	friend class CSQLColumn<CON>;
	friend class CSQLTable<CON>;
	friend class CSQLDatabase<CON>;

	/// Database connection.
	CON &con;

	/// Map of database tables.
	basics::map< basics::string<>, CSQLTable<CON> * > tables;

public:
	///////////////////////////////////////////////////////////////////////////
	/// Constructor.
	/// @param conn Database connection
	CSQLDatabase(CON &con)
		: con(con)
	{ }

	/// Destructor.
	/// Deletes the tables.
	~CSQLDatabase()
	{
		typename basics::map< basics::string<>, CSQLTable<CON> * >::iterator it(tables);
		for( ; it ; ++it )
			delete it->data;
		tables.clear();
	}

	///////////////////////////////////////////////////////////////////////////
	/// Creates a new child table.
	/// @param name Table name
	CSQLTable<CON> &table(const basics::string<> &name)
	{
		CSQLTable<CON> *tbl = new CSQLTable<CON>(*this,name);
		tables.insert(name,tbl);
		return *tbl;
	}

	/// Retrieves a child table.
	/// @param name Table name
	CSQLTable<CON> &operator[](const basics::string<> &name) throw(basics::exception)
	{
		if( tables.exists(name) )
			return *tables[name];
		else
		{
			basics::string<> err;
			err << "CSQLDatabase: Table '" << name << "' not found.";
			throw basics::exception(err); //## TODO: exception/exit/NULL?
		}
	}

	///////////////////////////////////////////////////////////////////////////
	/// Verifies that the database in the connection matches this one.
	/// If differences exist, the connection db is updated.
	/// @return If the databases matched
	bool verify();

	/// Rebuilds the database, removing all previous data.
	/// @return If it was successfull
	bool rebuild();

};
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
